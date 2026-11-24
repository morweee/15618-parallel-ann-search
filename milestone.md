# Milestone Report: Parallel Graph-Based Approximate Nearest-Neighbor Search on GPUs
**Team:** Mu-Wei Hsieh, Ryder (ShengHua) Chou  
**Project webpage:** <https://github.com/morweee/15618-parallel-ann-search>

## Summary of Work Completed

We have built the full data and graph pipeline on the GHC machines. A Python script converts SIFT1M (1M image-feature vectors with known true neighbors) from the ANN-Benchmarks HDF5 file (a multi-array format that needs a special library to read) into flat binary files and splits the 10K queries into 1K validation queries (for choosing search settings) and 9K test queries (for reported results). A C++ tool builds an hnswlib index (M = 16, ef_construction = 200; 83 s on 8 cores) and exports its bottom layer as a fixed-width 1M × 32 adjacency array padded with −1. Our checks confirm that there are no invalid ids, the average degree is 21.2, and 999,999 of 1,000,000 vertices are reachable from the entry point.

We also wrote the serial reference search, which is a best-first search with a sorted candidate list of size L and a visited array that is reset by incrementing a counter. The OpenMP version reuses it, and the CUDA versions will be checked against it for correctness. Using it, we completed the graph-feasibility check (next section).

## Preliminary Results: Graph-Feasibility Check

The main risk in our proposal was that without hnswlib's upper layers, searching only the bottom layer might need too many steps to reach good recall. Our experiments on the GHC machines show that this is not a problem. On the 1K validation queries, the bottom-layer search reaches recall@10 = 0.96 at L = 60 and 0.985 at L = 100, essentially identical to full hnswlib search at the same ef.

| L (ours) / ef (hnswlib) | Recall@10 (bottom layer only) | Recall@10 (full hnswlib) | Distance computations per query (ours) | Queries/s, 1 thread (ours) | Queries/s, 1 thread (hnswlib) |
|---|---|---|---|---|---|
| 20 | 0.840 | 0.840 | 629 | 8,449 | 23,981 |
| 40 | 0.929 | 0.930 | 944 | 5,577 | 14,231 |
| 60 | **0.961** | 0.962 | 1,247 | 4,138 | 9,657 |
| 100 | **0.985** | 0.985 | 1,822 | 2,743 | 6,549 |
| 200 | 0.996 | 0.996 | 3,122 | 1,521 | 3,602 |

We will use L = 60 as the main setting (recall ≥ 0.95) and L = 100 as a high-recall setting. At these settings, each query needs about 1,200 to 1,800 distance computations out of 1M vectors. Our serial search is about 2.3× slower per query than hnswlib, which uses SIMD distance code and prefetching and starts from a better entry point found in its upper layers. We will report speedups against both baselines.

## Progress Against Goals and Deliverables

- **Goal 1 (validate the graph and search):** done for SIFT1M. The GIST1M check is scheduled for Nov 27–29.
- **Goal 2 (four implementations):** serial is done. OpenMP is nearly complete (97%), and the thread-per-query and warp-per-query CUDA versions are in progress.
- **Goal 3 (experiments):** will begin once the remaining versions are done.

We are slightly behind our proposal schedule. The OpenMP and thread-per-query CUDA versions will be done by Nov 26, and the warp-per-query version and initial profiling by Dec 2 (shown in the updated schedule). We still expect to deliver everything in the proposal's "plan to achieve" list, because the graph check passed without needing a fallback and the remaining parallel versions reuse the same loader, graph format, and reference search. We will work on the "hope to achieve" items (multiple warps per query, a shared-memory visited set) only if the core experiments are complete by Dec 6. If time runs short, we will reduce the GIST1M experiments first, as stated in the proposal.

**Goals for the poster session:**

1. Serial, OpenMP, thread-per-query CUDA, and warp-per-query CUDA versions, all checked against the serial results.
2. Recall vs. throughput curves for all four versions on SIFT1M.
3. Batch-size results (1, 32, 256, 1K, all test queries) for both GPU versions.
4. Nsight Compute profiling of both kernels at batch 32 and full batch, with an explanation of the main bottleneck.
5. SIFT1M vs. GIST1M comparison at a common batch size.

## What We Plan to Show at the Poster Session

We will present a regular poster. It will show a diagram of the two GPU mappings (one thread per query vs. one warp per query), recall vs. throughput curves, throughput and latency vs. batch size, and a table of profiling metrics (memory throughput, occupancy, divergence) that explains where each design is limited.

## Concerns and Remaining Unknowns

- **Visited sets on the GPU.** A 1M-bit visited set is 125 KB per query, so running all 9K test queries at once needs about 1.1 GB. We plan to either process queries in chunks or use a small hash table per query, and report the choice.
- **Memory access and divergence.** Neighbor vectors are scattered in memory, and queries finish after different numbers of steps. We expect thread-per-query to suffer from uncoordinated memory accesses, but we do not yet know how large the effect is.
- **Fair CPU baseline.** Our serial search is slower than hnswlib. We may add SIMD distance code to the CPU versions so the CPU vs. GPU comparison is fair.

<!-- pagebreak -->

## Updated Schedule

| Dates | Mu-Wei Hsieh | Ryder Chou |
|------|--------------|--------------|
| Nov 24–26 | 1. OpenMP version<br>2. Verify results match serial<br>3. CPU thread scaling (1/2/4/8 threads) | 1. Thread-per-query CUDA kernel<br>2. Decide visited-set storage<br>3. Check recall matches serial |
| Nov 27–29 | 1. GIST1M conversion and graph check<br>2. Experiment scripts (CSV output)<br>3. Plotting script | 1. First GPU timings<br>2. Test block sizes 64/128/256<br>3. Start warp-per-query kernel |
| Nov 30–Dec 2 | 1. Experiment A (recall vs. throughput) for CPU and thread-per-query<br>2. Experiment B (batch size) for CPU and thread-per-query | 1. Finish and verify warp-per-query kernel<br>2. Nsight Compute profiling (experiment D) |
| Dec 3–6 | 1. Experiment C (SIFT1M vs. GIST1M)<br>2. Final plots | 1. Bottleneck analysis from profiling<br>2. Stretch goals, if core results are done |
| Dec 7–9 | 1. Final report: results and analysis sections | 1. Final report: GPU design and profiling sections |
| Dec 10–11 | 1. Submit report (Dec 10)<br>2. Poster<br>3. Submit source code to Autolab | 1. Submit report (Dec 10)<br>2. Poster<br>3. Poster session (Dec 11, 8:30am) |
