// Build/load an hnswlib index, export its bottom layer as a fixed-width
// adjacency array and run sanity checks

// baseline: full-hnswlib recall
//
// Usage: export_graph DATA_DIR [M=16] [ef_construction=200]
//   reads  DATA_DIR/base.fvecs, query_val.fvecs, gt_val.ivecs
//   writes DATA_DIR/hnsw_M{M}_ef{efc}.index  (reused if it already exists)
//          DATA_DIR/graph_M{M}_ef{efc}.bin   (format in src/common.h)
//          results/hnsw_baseline_M{M}_ef{efc}.csv
#include <algorithm>
#include <queue>
#include <sys/stat.h>

#include "../src/common.h"
#include "hnswlib/hnswlib.h"

#ifdef _OPENMP
#include <omp.h>
#endif

static bool file_exists(const std::string &p) {
    struct stat st;
    return stat(p.c_str(), &st) == 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s DATA_DIR [M] [ef_construction]\n", argv[0]);
        return 1;
    }
    std::string dir = argv[1];
    int M = argc > 2 ? std::atoi(argv[2]) : 16;
    int efc = argc > 3 ? std::atoi(argv[3]) : 200;
    std::string tag = "M" + std::to_string(M) + "_ef" + std::to_string(efc);
    std::string index_path = dir + "/hnsw_" + tag + ".index";
    std::string graph_path = dir + "/graph_" + tag + ".bin";

    auto base = read_vecs<float>(dir + "/base.fvecs");
    std::printf("base: %zu x %zu\n", base.n, base.d);

    hnswlib::L2Space space(base.d);
    hnswlib::HierarchicalNSW<float> *index;
    if (file_exists(index_path)) {
        std::printf("loading %s\n", index_path.c_str());
        index = new hnswlib::HierarchicalNSW<float>(&space, index_path);
    } else {
        std::printf("building index (M=%d, ef_construction=%d)...\n", M, efc);
        double t0 = now_sec();
        index = new hnswlib::HierarchicalNSW<float>(&space, base.n, M, efc);
        // Label = row index in base.fvecs. Internal ids may differ when built in parallel.
        index->addPoint(base.row(0), 0);
#pragma omp parallel for schedule(dynamic, 1024)
        for (long i = 1; i < (long)base.n; i++) index->addPoint(base.row(i), i);
        std::printf("built in %.1f s\n", now_sec() - t0);
        index->saveIndex(index_path);
    }

    // Export level 0, rewritten in label space so row i is base vector i.
    Graph g;
    g.n = (int32_t)index->cur_element_count;
    g.width = (int32_t)index->maxM0_;
    g.adj.assign((size_t)g.n * g.width, -1);
    long total_deg = 0;
    int max_deg = 0, bad_ids = 0;
    for (int32_t in = 0; in < g.n; in++) {
        int32_t v = (int32_t)index->getExternalLabel(in);
        if (v < 0 || v >= g.n) { bad_ids++; continue; }
        hnswlib::linklistsizeint *ll = index->get_linklist0(in);
        int cnt = index->getListCount(ll);
        auto *nb = (hnswlib::tableint *)(ll + 1);
        int32_t *row = g.adj.data() + (size_t)v * g.width;
        for (int j = 0; j < cnt; j++) {
            if (nb[j] >= (hnswlib::tableint)g.n) { bad_ids++; continue; }
            row[j] = (int32_t)index->getExternalLabel(nb[j]);
        }
        total_deg += cnt;
        max_deg = std::max(max_deg, cnt);
    }
    g.entry = (int32_t)index->getExternalLabel(index->enterpoint_node_);

    // Reachability from the entry point using only bottom-layer edges.
    std::vector<char> seen(g.n, 0);
    std::vector<int32_t> stack = {g.entry};
    seen[g.entry] = 1;
    long reached = 1;
    while (!stack.empty()) {
        int32_t v = stack.back();
        stack.pop_back();
        const int32_t *nb = g.neighbors(v);
        for (int j = 0; j < g.width && nb[j] >= 0; j++)
            if (!seen[nb[j]]) { seen[nb[j]] = 1; reached++; stack.push_back(nb[j]); }
    }

    std::printf("graph: n=%d width=%d entry=%d\n", g.n, g.width, g.entry);
    std::printf("degree: avg %.2f, max %d; bad ids %d\n", (double)total_deg / g.n, max_deg, bad_ids);
    std::printf("reachable from entry: %ld / %d (%.4f%%)\n", reached, g.n, 100.0 * reached / g.n);
    write_graph(graph_path, g);
    std::printf("wrote %s\n", graph_path.c_str());

    // Baseline: full hnswlib search (all layers) on the validation queries.
    auto q = read_vecs<float>(dir + "/query_val.fvecs");
    auto gt = read_vecs<int32_t>(dir + "/gt_val.ivecs");
    const int k = 10;
    std::string csv = "results/hnsw_baseline_" + tag + ".csv";
    FILE *out = std::fopen(csv.c_str(), "w");
    if (!out) { std::perror(csv.c_str()); return 1; }
    std::fprintf(out, "ef,recall_at_10,qps_1thread\n");
    std::printf("\nfull hnswlib search, %zu validation queries, 1 thread\n", q.n);
    std::printf("%6s %10s %10s\n", "ef", "recall@10", "QPS");
    for (int ef : {10, 20, 40, 60, 80, 100, 150, 200}) {
        index->setEf(ef);
        double rsum = 0, t0 = now_sec();
        for (size_t i = 0; i < q.n; i++) {
            auto pq = index->searchKnn(q.row(i), k);
            int32_t res[k];
            std::fill(res, res + k, -1);
            while (pq.size() > (size_t)k) pq.pop();
            for (int j = (int)pq.size() - 1; j >= 0; j--) { res[j] = (int32_t)pq.top().second; pq.pop(); }
            rsum += recall_at_k(res, gt.row(i), k);
        }
        double dt = now_sec() - t0;
        std::printf("%6d %10.4f %10.0f\n", ef, rsum / q.n, q.n / dt);
        std::fprintf(out, "%d,%.5f,%.1f\n", ef, rsum / q.n, q.n / dt);
    }
    std::fclose(out);
    std::printf("wrote %s\n", csv.c_str());
    delete index;
    return 0;
}
