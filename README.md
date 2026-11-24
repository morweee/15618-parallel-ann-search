# Parallel Graph-Based Approximate Nearest-Neighbor Search on GPUs

**15-418/618, Fall 2026 final project**  
**Team:** Mu-Wei Hsieh, Ryder (ShengHua) Chou

We are going to implement graph-based nearest-neighbor search on both GPU and multi-core CPU platforms, and analyze how different ways of mapping queries onto GPU threads affect performance.

## Reports

- [Project Proposal](proposal.md) ([PDF](proposal.pdf)), Nov 9
- [Milestone Report](milestone.md) ([PDF](milestone.pdf))
- Final Report, due Dec 10

## Schedule

Completed: data conversion, graph export, serial search, and the SIFT1M graph-feasibility check (see the milestone report).

| Dates | Mu-Wei Hsieh | Ryder Chou |
|------|--------------|--------------|
| Nov 24–26 | 1. OpenMP version<br>2. Verify results match serial<br>3. CPU thread scaling (1/2/4/8 threads) | 1. Thread-per-query CUDA kernel<br>2. Decide visited-set storage<br>3. Check recall matches serial |
| Nov 27–29 | 1. GIST1M conversion and graph check<br>2. Experiment scripts (CSV output)<br>3. Plotting script | 1. First GPU timings<br>2. Test block sizes 64/128/256<br>3. Start warp-per-query kernel |
| Nov 30–Dec 2 | 1. Experiment A (recall vs. throughput) for CPU and thread-per-query<br>2. Experiment B (batch size) for CPU and thread-per-query | 1. Finish and verify warp-per-query kernel<br>2. Nsight Compute profiling (experiment D) |
| Dec 3–6 | 1. Experiment C (SIFT1M vs. GIST1M)<br>2. Final plots | 1. Bottleneck analysis from profiling<br>2. Stretch goals, if core results are done |
| Dec 7–9 | 1. Final report: results and analysis sections | 1. Final report: GPU design and profiling sections |
| Dec 10–11 | 1. Submit report (Dec 10)<br>2. Poster<br>3. Submit source code to Autolab | 1. Submit report (Dec 10)<br>2. Poster<br>3. Poster session (Dec 11, 8:30am) |

Detailed task checklist and work split: [PLAN.md](PLAN.md)

## Running the code

On a GHC machine:

```
make graph-check    # downloads SIFT1M (~500 MB), converts it, builds and exports the graph, sweeps L
```

Outputs go to `results/`. On macOS without OpenMP, build with `make OMP=`.
