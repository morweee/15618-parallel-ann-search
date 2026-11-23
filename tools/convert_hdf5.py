#!/usr/bin/env python3
"""Convert an ANN-Benchmarks HDF5 file into flat .fvecs/.ivecs files.

Writes into OUT_DIR:
  base.fvecs        base vectors (N x d)
  query_val.fvecs   validation queries (used to pick L)
  query_test.fvecs  test queries (used for reported results)
  gt_val.ivecs      true neighbors of the validation queries (k = 100)
  gt_test.ivecs     true neighbors of the test queries
  split.ivecs       one row: original indices of the validation queries, then the test queries

Usage: python3 tools/convert_hdf5.py data/sift-128-euclidean.hdf5 data/sift --val 1000
"""
import argparse
import os

import h5py
import numpy as np


def write_vecs(path, arr, dtype):
    arr = np.ascontiguousarray(arr, dtype=dtype)
    n, d = arr.shape
    out = np.empty((n, d + 1), dtype=np.int32)
    out[:, 0] = d
    out[:, 1:] = arr.view(np.int32)
    out.tofile(path)
    print(f"  {path}: {n} x {d}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("hdf5")
    ap.add_argument("out_dir")
    ap.add_argument("--val", type=int, default=1000, help="number of validation queries")
    ap.add_argument("--seed", type=int, default=618)
    args = ap.parse_args()

    os.makedirs(args.out_dir, exist_ok=True)
    with h5py.File(args.hdf5, "r") as f:
        if f.attrs.get("distance", "euclidean") != "euclidean":
            raise SystemExit("only euclidean datasets are supported")
        base = f["train"][:]
        queries = f["test"][:]
        gt = f["neighbors"][:]

    rng = np.random.default_rng(args.seed)
    perm = rng.permutation(len(queries))
    val, test = perm[: args.val], perm[args.val :]

    print(f"base {base.shape}, queries {queries.shape}, gt {gt.shape}")
    write_vecs(os.path.join(args.out_dir, "base.fvecs"), base, np.float32)
    write_vecs(os.path.join(args.out_dir, "query_val.fvecs"), queries[val], np.float32)
    write_vecs(os.path.join(args.out_dir, "query_test.fvecs"), queries[test], np.float32)
    write_vecs(os.path.join(args.out_dir, "gt_val.ivecs"), gt[val], np.int32)
    write_vecs(os.path.join(args.out_dir, "gt_test.ivecs"), gt[test], np.int32)
    write_vecs(os.path.join(args.out_dir, "split.ivecs"), perm[None, :], np.int32)


if __name__ == "__main__":
    main()
