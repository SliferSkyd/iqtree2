**hPartition** is a partitioning framework implemented natively inside IQ-TREE 2, which builds a partitioning scheme from site rates rather than from predefined gene or codon boundaries, and does so at a fraction of the time and memory that wrapper-based methods need.

### 🔄 Changes since hPartition-v1.2
Codon alignments now run. Three faults were fixed, none of which changes any previously reported result:

* the internal IQ-TREE invocations did not pass on the sequence type, so codon data was re-detected from the file and taken for nucleotide data;
* schemes were written in codon indices, where the engine's partition reader expects nucleotide coordinates for codon data;
* the buffer exporting a subset's rate matrix was fixed at the 400 entries of an amino-acid matrix, which a 61-state codon model overran, aborting any partitioned codon run.

Schemes built from nucleotide, binary and morphological data are byte-identical to those of v1.2.

### ✨ Key Features
* **Native IQ-TREE 2 Integration:** Implemented directly in C++ within the IQ-TREE 2 engine. Model fitting, site-wise likelihood evaluation and reassignment share one process and one in-memory alignment, and the framework inherits the engine's alignment and state-space handling rather than reimplementing it.
* **Hybrid Partitioning Strategy:** Model-free evolutionary rate estimation via a generalized `fastTIGER`, then seed binning, candidate-model selection, likelihood-based site reassignment, and a cleanup pass over undersized subsets.
* **Data Types:** Benchmarked on nucleotide and amino-acid alignments, and verified to run unchanged on binary, morphological and codon data. Only the nucleotide and amino-acid results are benchmarks; the other three are checks that the pipeline runs.
* **Invariant Site Handling:** Invariant sites are carried through the pipeline at rate zero and settled by likelihood-proportional sampling, so they disperse across the scheme instead of collecting in one zero-variance cluster. Small entirely invariant subsets still occur — in five of the eight DNA schemes examined — but no subset holds more than 11.8% of an alignment's invariant sites.
* **hPartition-PF:** An extended variant (`-hPartition-pf`) that passes the scheme to IQ-TREE's relaxed-clustering merge search. It produces markedly smaller schemes on every DNA dataset tested, at 1.2 to 13.9 times the runtime, and improves BIC on 11 of 17.

### 📊 Benchmark Results
* **Model fit:** Lowest BIC of the methods compared on 16 of 17 empirical DNA alignments, 10 of 12 protein alignments, and three of four multi-locus alignments containing ribosomal or transfer RNA.
* **Topological accuracy:** On 100 simulated alignments per dataset, hPartition recovers a closer tree than gPartition on 56 and than PsiPartition on 66 (SimDNA1), and on 70 and 85 respectively (SimDNA2); all four comparisons are significant under a two-sided Wilcoxon signed-rank test. The advantage is consistent rather than concentrated, but it is not universal.
* **Speed:** Against the fastest competitor on each alignment, 1.0–6.4× lower wall-clock time on DNA and 2.4–25.7× on protein.
* **Scalability:** Completes alignments on which the alternatives exceed a 10 GB memory allocation.

### 🧪 Benchmark Scripts & Reproducibility
The scripts that produced every experiment in the manuscript are in `hPartition_scripts/`, with `revision/` holding those added during review (the RNA benchmark, the matched-model-set comparison, the Stage-2 ablation and the binary-data check). `hPartition_scripts/README.md` documents the two clusters the work ran on, how to point the scripts at your own binaries, and what each one produces.

### 🚀 Usage

To partition an alignment with standard **hPartition**:

```bash
iqtree2 -s ALIGNMENT_FILE -mset JC69,F81,HKY,GTR -hPartition   # DNA
iqtree2 -s ALIGNMENT_FILE -hPartition                          # protein
```

For the merging extension **hPartition-PF**, append `-pf`:

```bash
iqtree2 -s ALIGNMENT_FILE -mset JC69,F81,HKY,GTR -hPartition-pf   # DNA
```

The criterion defaults to BIC; `--AIC` and `--AICc` select the others. The scheme depends on the random seed, so hold it fixed within an analysis: every result reported in the manuscript was obtained at `-seed 0` on a single core.

### 🛠️ Compilation from Source
Pre-compiled binaries for Linux, macOS, and Windows are in the assets below. To compile from source:

> **⚠️ IMPORTANT:** Do **not** use the "Source code (zip)" or "Source code (tar.gz)" links GitHub generates at the bottom of this page — they omit the required Git submodules. Download the `iqtree-...-source-with-submodules` archive from the assets instead.

**Requirements (Linux):**
* A C++ compiler (GCC >= 4.8, Clang, or Intel C++)
* CMake >= 2.8.10
* Boost, Eigen3 and OpenMP (e.g. `sudo apt-get install libboost-dev libeigen3-dev`)

```bash
# 1. Extract the source archive and enter the directory
unzip iqtree-<version>-source-with-submodules.zip
cd iqtree2

# 2. Build
mkdir build && cd build
cmake ..
make -j
```

The `iqtree2` executable is left in the build folder.

*Troubleshooting:* if `cmake ..` reports that Eigen3 is missing, install it and retry; if it still fails, give the path explicitly:

```bash
cmake -DEIGEN3_INCLUDE_DIR=<eigen3_dir> ..
make -j
```

**macOS, Windows, and advanced builds:** see the official IQ-TREE compilation guide for macOS, Windows, MPI, ARM, and Xcode builds.
