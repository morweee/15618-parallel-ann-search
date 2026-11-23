// Graph-feasibility check: serial bottom-layer-only search, sweeping L.
//
// Usage: graph_check DATA_DIR GRAPH_FILE [val|test] [out.csv]
// Prints and writes L, recall@10, average distance computations per query, QPS.
#include <cstring>

#include "search.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: %s DATA_DIR GRAPH_FILE [val|test] [out.csv]\n", argv[0]);
        return 1;
    }
    std::string dir = argv[1];
    std::string split = argc > 3 ? argv[3] : "val";
    std::string csv = argc > 4 ? argv[4] : "results/graph_check_" + split + ".csv";

    auto base = read_vecs<float>(dir + "/base.fvecs");
    auto q = read_vecs<float>(dir + "/query_" + split + ".fvecs");
    auto gt = read_vecs<int32_t>(dir + "/gt_" + split + ".ivecs");
    Graph g = read_graph(argv[2]);
    if ((size_t)g.n != base.n || q.n != gt.n) {
        std::fprintf(stderr, "size mismatch: graph %d, base %zu, queries %zu, gt %zu\n",
                     g.n, base.n, q.n, gt.n);
        return 1;
    }
    std::printf("base %zu x %zu, %zu %s queries, graph width %d, entry %d\n",
                base.n, base.d, q.n, split.c_str(), g.width, g.entry);

    const int k = 10;
    FILE *out = std::fopen(csv.c_str(), "w");
    if (!out) { std::perror(csv.c_str()); return 1; }
    std::fprintf(out, "L,recall_at_10,avg_dist_computations,qps_1thread\n");
    std::printf("%5s %10s %12s %10s\n", "L", "recall@10", "dists/query", "QPS");

    Searcher s(g, base);
    std::vector<int32_t> res(k);
    for (int L : {10, 20, 40, 60, 80, 100, 120, 150, 200}) {
        s.dist_count = 0;
        double rsum = 0, t0 = now_sec();
        for (size_t i = 0; i < q.n; i++) {
            s.search(q.row(i), L, k, res.data());
            rsum += recall_at_k(res.data(), gt.row(i), k);
        }
        double dt = now_sec() - t0;
        double recall = rsum / q.n, dpq = (double)s.dist_count / q.n, qps = q.n / dt;
        std::printf("%5d %10.4f %12.0f %10.0f\n", L, recall, dpq, qps);
        std::fprintf(out, "%d,%.5f,%.1f,%.1f\n", L, recall, dpq, qps);
    }
    std::fclose(out);
    std::printf("wrote %s\n", csv.c_str());
    return 0;
}
