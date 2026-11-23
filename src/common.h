// Shared loaders, graph format and recall helper.
#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

// Row-major matrix read from .fvecs / .ivecs (each row: int32 dim, then dim values).
template <typename T>
struct Matrix {
    std::vector<T> data;
    size_t n = 0, d = 0;
    const T *row(size_t i) const { return data.data() + i * d; }
};

template <typename T>
Matrix<T> read_vecs(const std::string &path) {
    FILE *f = std::fopen(path.c_str(), "rb");
    if (!f) { std::perror(path.c_str()); std::exit(1); }
    int32_t dim;
    if (std::fread(&dim, 4, 1, f) != 1) { std::fprintf(stderr, "%s: empty\n", path.c_str()); std::exit(1); }
    std::fseek(f, 0, SEEK_END);
    size_t bytes = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    size_t row_bytes = 4 + (size_t)dim * sizeof(T);
    Matrix<T> m;
    m.d = dim;
    m.n = bytes / row_bytes;
    m.data.resize(m.n * m.d);
    for (size_t i = 0; i < m.n; i++) {
        int32_t di;
        if (std::fread(&di, 4, 1, f) != 1 || di != dim ||
            std::fread(m.data.data() + i * m.d, sizeof(T), m.d, f) != m.d) {
            std::fprintf(stderr, "%s: bad row %zu\n", path.c_str(), i);
            std::exit(1);
        }
    }
    std::fclose(f);
    return m;
}

// Bottom-layer graph: fixed-width adjacency, padded with -1.
// File layout: int32 n, int32 width, int32 entry, then n * width int32 neighbor ids.
struct Graph {
    int32_t n = 0, width = 0, entry = 0;
    std::vector<int32_t> adj;
    const int32_t *neighbors(int32_t v) const { return adj.data() + (size_t)v * width; }
};

inline void write_graph(const std::string &path, const Graph &g) {
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) { std::perror(path.c_str()); std::exit(1); }
    int32_t hdr[3] = {g.n, g.width, g.entry};
    std::fwrite(hdr, 4, 3, f);
    std::fwrite(g.adj.data(), 4, g.adj.size(), f);
    std::fclose(f);
}

inline Graph read_graph(const std::string &path) {
    FILE *f = std::fopen(path.c_str(), "rb");
    if (!f) { std::perror(path.c_str()); std::exit(1); }
    Graph g;
    int32_t hdr[3];
    if (std::fread(hdr, 4, 3, f) != 3) { std::fprintf(stderr, "%s: bad header\n", path.c_str()); std::exit(1); }
    g.n = hdr[0]; g.width = hdr[1]; g.entry = hdr[2];
    g.adj.resize((size_t)g.n * g.width);
    if (std::fread(g.adj.data(), 4, g.adj.size(), f) != g.adj.size()) {
        std::fprintf(stderr, "%s: truncated\n", path.c_str()); std::exit(1);
    }
    std::fclose(f);
    return g;
}

inline float l2sq(const float *a, const float *b, size_t d) {
    float s = 0.f;
    for (size_t i = 0; i < d; i++) { float t = a[i] - b[i]; s += t * t; }
    return s;
}

// Fraction of the first k true neighbors that appear in result[0..k).
inline double recall_at_k(const int32_t *result, const int32_t *truth, int k) {
    int hit = 0;
    for (int i = 0; i < k; i++)
        for (int j = 0; j < k; j++)
            if (result[i] == truth[j]) { hit++; break; }
    return (double)hit / k;
}

inline double now_sec() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}
