# Development Plan

Working checklist for the project. Check items off as they are done and update the timeline if plans change.

**Key dates:** Milestone report Nov 23 · Final report Dec 10 · Poster session Dec 11 (618 slot, 8:30am)

**Critical path:** graph export + graph-feasibility check (sections 3–4) should be done by **Nov 18**. Everything in sections 5–7 depends on the result.

## 1. Setup and tooling
- [ ] Both partners have push access to the repo
- [ ] Repo layout: `src/` (shared loader, recall), `cpu/`, `cuda/`, `tools/` (graph export), `scripts/` (experiments, plots), `results/`
- [ ] One Makefile that builds on GHC with CUDA 11.7 (`-arch=sm_75`), plus an `-arch=sm_89` target for the RTX 4080 laptop
- [ ] Shared binary file format (small header + raw arrays) read by every program
- [ ] Timing helper: CUDA events for GPU work, wall clock for full runs, results written to CSV

## 2. Data
- [ ] Download `sift-128-euclidean.hdf5` from ANN-Benchmarks (~500 MB; base vectors, queries, true neighbors)
- [ ] Python script converting HDF5 to flat `.fvecs` / `.ivecs` files, so C++ needs no HDF5 library
- [ ] Fixed split of SIFT's 10K queries: validation set (e.g. 1K, for choosing L) and test set (9K, for reported results); save the split
- [ ] GIST1M (~3.6 GB) later: check GHC disk quota first; may need a machine's local disk

## 3. Graph export
- [ ] Small C++ program using hnswlib (header-only) to build the index and save it
- [ ] Read each vertex's bottom-layer neighbors with `get_linklist0(i)` (a count followed by up to 2M ids)
- [ ] Write a fixed-width `N × 2M` int32 adjacency array (padded with −1), plus the entry point (`enterpoint_node_`)
- [ ] Map hnswlib internal ids to labels (`getExternalLabel`, or insert items in order so they match), otherwise recall will be wrong
- [ ] Sanity checks: average and maximum neighbor count, no out-of-range ids, every vertex reachable from the entry point (BFS)

## 4. Graph-feasibility check
- [ ] Serial reference search: sorted candidate list of size L, visited array reset by bumping a counter, recall@10
- [ ] Sweep L = 10…200 on the validation queries; record recall@10 and number of distance calculations
- [ ] Compare against full hnswlib search at the same `ef`
- [ ] **Decision:** if bottom layer alone reaches recall@10 ≥ 0.95 at L ≤ 200, continue. Otherwise try fallbacks in order: better start vertex → denser graph → CPU upper layers + GPU bottom layer

## 5. CPU versions
- [ ] Serial version (from section 4)
- [ ] OpenMP version: `parallel for` over queries, `schedule(dynamic)`, per-thread scratch, graph shared read-only
- [ ] Check OpenMP results match serial exactly, query by query

## 6. GPU versions
- [ ] Decide where visited sets live (a 1M-bit set is 125 KB per query; 10K queries at once = 1.25 GB): process queries in chunks, or a small hash table per running query. Record the choice for the report
- [ ] Thread-per-query kernel: graph in global memory, each thread's candidate list in local or global memory; check recall matches serial at the same L
- [ ] Warp-per-query kernel: lanes split each distance (4 floats per lane for 128-d, combined with shuffles) or split the neighbors; one lane or the whole warp updates the candidate list
- [ ] Test block sizes (64 / 128 / 256) before the main experiments
- [ ] Run `nvidia-smi` before every timing run (GHC GPUs are shared)

## 7. Experiments and profiling
- [ ] A: recall vs throughput (sweep L)
- [ ] B: batch size 1 / 32 / 256 / 1K / full, search settings fixed
- [ ] C: SIFT1M vs GIST1M at a common batch size
- [ ] D: `ncu` profiling at batch 32 and full batch (use the `profile.sh` wrapper)
- [ ] One script per experiment writing CSV, plus one plotting script
- [ ] Report GPU kernel time separately from total time (including copies); load the graph onto the GPU once

## 8. Deliverables
- [ ] **Milestone (Nov 23):** progress, graph-check result, early CPU and thread-per-query timings, updated schedule, concerns. Add to README as Markdown + PDF
- [ ] Final report (Dec 10)
- [ ] Poster (Dec 11)

## Timeline and split

| When | Morris | Ryder |
|---|---|---|
| Nov 16–18 | Data conversion, graph export, serial search, graph check | Repo layout, Makefile, timing helper, CUDA starting code |
| Nov 19–21 | OpenMP version, CPU timings | Thread-per-query kernel, correctness check |
| Nov 22–23 | Milestone write-up | Initial GPU timings for the milestone |
| Nov 24–29 | Start GIST1M | Warp-per-query kernel, first profiling |
| Nov 30–Dec 6 | Experiments A–D, plots | Experiments A–D, bottleneck analysis |
| Dec 7–11 | Final report, poster | Final report, poster |
