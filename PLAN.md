# Development Plan

Working checklist for the project. Check items off as they are done and update the timeline if plans change.

**Key dates:** Milestone report Nov 23 · Final report Dec 10 · Poster session Dec 11 (618 slot, 8:30am)

**Critical path:** graph export + graph-feasibility check (sections 3–4) should be done by **Nov 18**. Everything in sections 5–7 depends on the result.

## 1. Setup and tooling
- [ ] Both partners have push access to the repo
- [ ] Repo layout: `src/` (shared loader, recall), `cpu/`, `cuda/`, `tools/` (graph export), `scripts/` (experiments, plots), `results/`
- [ ] One Makefile that builds on GHC with CUDA 11.7 (`-arch=sm_75`), plus an `-arch=sm_89` target for the RTX 4080 laptop
- [x] Shared binary file format (small header + raw arrays) read by every program
- [ ] Timing helper: CUDA events for GPU work, wall clock for full runs, results written to CSV

## 2. Data
- [x] Download `sift-128-euclidean.hdf5` from ANN-Benchmarks (~500 MB; base vectors, queries, true neighbors)
- [x] Python script converting HDF5 to flat `.fvecs` / `.ivecs` files, so C++ needs no HDF5 library
- [x] Fixed split of SIFT's 10K queries: validation set (e.g. 1K, for choosing L) and test set (9K, for reported results); save the split
- [ ] GIST1M (~3.6 GB) later: check GHC disk quota first; may need a machine's local disk

## 3. Graph export
- [x] Small C++ program using hnswlib (header-only) to build the index and save it
- [x] Read each vertex's bottom-layer neighbors with `get_linklist0(i)` (a count followed by up to 2M ids)
- [x] Write a fixed-width `N × 2M` int32 adjacency array (padded with −1), plus the entry point (`enterpoint_node_`)
- [x] Map hnswlib internal ids to labels (`getExternalLabel`, or insert items in order so they match), otherwise recall will be wrong
- [x] Sanity checks: average and maximum neighbor count, no out-of-range ids, every vertex reachable from the entry point (BFS)

## 4. Graph-feasibility check
- [x] Serial reference search: sorted candidate list of size L, visited array reset by bumping a counter, recall@10
- [x] Sweep L = 10…200 on the validation queries; record recall@10 and number of distance calculations
- [x] Compare against full hnswlib search at the same `ef`
- [x] **Decision:** if bottom layer alone reaches recall@10 ≥ 0.95 at L ≤ 200, continue. Otherwise try fallbacks in order: better start vertex → denser graph → CPU upper layers + GPU bottom layer
  - **Result (Nov 24):** passed. Bottom layer alone reaches recall@10 = 0.961 at L = 60 and 0.985 at L = 100, matching full hnswlib. No fallback needed. See `results/` and the milestone report.

## 5. CPU versions
- [x] Serial version (from section 4)
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

Same as the schedule in the milestone report.

| Dates | Morris | Ryder |
|------|--------------|--------------|
| Nov 24–26 | 1. OpenMP version<br>2. Verify results match serial<br>3. CPU thread scaling (1/2/4/8 threads) | 1. Thread-per-query CUDA kernel<br>2. Decide visited-set storage<br>3. Check recall matches serial |
| Nov 27–29 | 1. GIST1M conversion and graph check<br>2. Experiment scripts (CSV output)<br>3. Plotting script | 1. First GPU timings<br>2. Test block sizes 64/128/256<br>3. Start warp-per-query kernel |
| Nov 30–Dec 2 | 1. Experiment A (recall vs. throughput) for CPU and thread-per-query<br>2. Experiment B (batch size) for CPU and thread-per-query | 1. Finish and verify warp-per-query kernel<br>2. Nsight Compute profiling (experiment D) |
| Dec 3–6 | 1. Experiment C (SIFT1M vs. GIST1M)<br>2. Final plots | 1. Bottleneck analysis from profiling<br>2. Stretch goals, if core results are done |
| Dec 7–9 | 1. Final report: results and analysis sections | 1. Final report: GPU design and profiling sections |
| Dec 10–11 | 1. Submit report (Dec 10)<br>2. Poster<br>3. Submit source code to Autolab | 1. Submit report (Dec 10)<br>2. Poster<br>3. Poster session (Dec 11, 8:30am) |
