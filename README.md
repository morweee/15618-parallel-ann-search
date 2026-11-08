# Parallel Graph-Based Approximate Nearest-Neighbor Search on GPUs

**15-418/618, Fall 2026 final project**  
**Team:** Mu-Wei Hsieh, Ryder (ShengHua) Chou

We are going to implement graph-based nearest-neighbor search on both GPU and multi-core CPU platforms, and analyze how different ways of mapping queries onto GPU threads affect performance.

## Reports

- [Project Proposal](proposal.md) ([PDF](proposal.pdf)), Nov 9
- Milestone Report, due Nov 23
- Final Report, due Dec 10

## Schedule

| Week | Planned work |
|------------|------------------------------------------------------------------------|
| 1 (Nov 9) | Submit the proposal on Nov 9. Prepare datasets, export the graph, and implement the serial reference. Complete the Nov 12 graph-feasibility checkpoint. |
| 2 (Nov 16) | Implement the OpenMP and thread-per-query CUDA versions. Check correctness, collect initial timings, and begin the warp version. Prepare the milestone report. |
| 3 (Nov 23) | Submit the milestone on Nov 23. Keep Thanksgiving week lighter: complete and check the warp version, run initial profiling, and finalize experiment scripts. |
| 4 (Nov 30) | Run the main experiments, investigate bottlenecks, and prepare plots. Attempt stretch goals only after core results are complete. |
| 5 (Dec 7) | Finish measurements, submit the report on Dec 10, and present the poster on Dec 11. |
