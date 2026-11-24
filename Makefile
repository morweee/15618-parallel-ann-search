# Builds on GHC (g++ with OpenMP). On macOS without libomp: make OMP=
CXX      ?= g++
OMP      ?= -fopenmp
CXXFLAGS ?= -O3 -march=native -std=c++17 -Wall
HNSWLIB  := third_party/hnswlib
DATA     ?= data/sift
M        ?= 16
EFC      ?= 200

BINS := bin/export_graph bin/graph_check

all: $(BINS)

$(HNSWLIB):
	git clone --depth 1 --branch v0.8.0 https://github.com/nmslib/hnswlib.git $(HNSWLIB)

bin/export_graph: tools/export_graph.cpp src/common.h | $(HNSWLIB)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(OMP) -I$(HNSWLIB) $< -o $@

bin/graph_check: src/graph_check.cpp src/search.h src/common.h
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $< -o $@

# Full milestone pipeline (expects data/sift-128-euclidean.hdf5).
data/sift-128-euclidean.hdf5:
	@mkdir -p data
	curl -L -o $@ http://ann-benchmarks.com/sift-128-euclidean.hdf5

$(DATA)/base.fvecs: data/sift-128-euclidean.hdf5
	python3 tools/convert_hdf5.py $< $(DATA) --val 1000

graph-check: $(BINS) $(DATA)/base.fvecs
	@mkdir -p results
	./bin/export_graph $(DATA) $(M) $(EFC)
	./bin/graph_check $(DATA) $(DATA)/graph_M$(M)_ef$(EFC).bin val results/graph_check_M$(M)_ef$(EFC)_val.csv

# Report PDFs, e.g. make milestone.pdf (needs pandoc and xelatex)
%.pdf: %.md docs/pdf-style.tex docs/pdf-filter.lua
	pandoc $< -o $@ --pdf-engine=xelatex -V geometry:margin=1in -V mainfont=Helvetica \
	  -V colorlinks=true -V urlcolor=blue -V linkcolor=blue \
	  -H docs/pdf-style.tex --lua-filter=docs/pdf-filter.lua

clean:
	rm -rf bin

.PHONY: all graph-check clean
