// Serial best-first (beam) search on the bottom-layer graph.
// Shared by the serial reference and the OpenMP version (one Searcher per thread).
#pragma once

#include "common.h"

struct Candidate {
    float dist;
    int32_t id;
    bool expanded;
};

struct Searcher {
    const Graph &g;
    const Matrix<float> &base;
    std::vector<uint32_t> visited;  // visited[v] == stamp  <=>  v seen in the current query
    uint32_t stamp = 0;
    std::vector<Candidate> list;    // sorted by dist, size <= L
    long dist_count = 0;            // distance computations, summed over queries

    Searcher(const Graph &g, const Matrix<float> &base) : g(g), base(base), visited(g.n, 0) {}

    // Writes the k nearest found ids (sorted) into out. Requires k <= L.
    void search(const float *q, int L, int k, int32_t *out) {
        if (++stamp == 0) {  // counter wrapped: reset once every 2^32 queries
            std::fill(visited.begin(), visited.end(), 0);
            stamp = 1;
        }
        list.clear();
        list.reserve(L + 1);
        int32_t ep = g.entry;
        visited[ep] = stamp;
        list.push_back({l2sq(q, base.row(ep), base.d), ep, false});
        dist_count++;

        size_t pos = 0;  // every candidate before pos has been expanded
        while (pos < list.size()) {
            if (list[pos].expanded) { pos++; continue; }
            list[pos].expanded = true;
            const int32_t *nb = g.neighbors(list[pos].id);
            size_t lowest_insert = list.size();
            for (int j = 0; j < g.width && nb[j] >= 0; j++) {
                int32_t v = nb[j];
                if (visited[v] == stamp) continue;
                visited[v] = stamp;
                float d = l2sq(q, base.row(v), base.d);
                dist_count++;
                if ((int)list.size() == L && d >= list.back().dist) continue;
                // Insert in sorted position.
                size_t i = list.size();
                list.push_back({d, v, false});
                while (i > 0 && list[i - 1].dist > d) { list[i] = list[i - 1]; i--; }
                list[i] = {d, v, false};
                if ((int)list.size() > L) list.pop_back();
                if (i < lowest_insert) lowest_insert = i;
            }
            // Continue from the closest unexpanded candidate.
            pos = std::min(pos + 1, lowest_insert);
        }
        for (int i = 0; i < k; i++) out[i] = i < (int)list.size() ? list[i].id : -1;
    }
};
