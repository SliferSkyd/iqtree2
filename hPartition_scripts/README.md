# hPartition Benchmark Scripts

This directory contains the scripts used to benchmark **hPartition**, **hPartition-PF**, **gPartition**, **mPartition**, and **PsiPartition** on both empirical and simulated datasets as described in our paper. The scripts are organized into two main folders: `empricial` (for empirical datasets) and `simulated` (for simulated datasets).
## Server Environment

All experiments related to these scripts were performed on a Linux cluster composed of homogeneous nodes, each featuring 28 cores running at 2.7 GHz. 

The scripts are designed to be submitted to a Portable Batch System (PBS) job scheduler via `qsub`. Each job script requests the following resources:
*   **Queue:** `para_cpu`
*   **Resources per job:** 1 CPU core, 1 OpenMP thread, 1 MPI process, and 10 GB of memory (`-l select=1:ncpus=1:ompthreads=1:mem=10G:mpiprocs=1`).

## Directory Structure

```
hPartition_scripts/
├── empricial/
│   ├── dna/
│   │   ├── gPartition.sh
│   │   ├── hPartition.sh
│   │   ├── hPartition-PF.sh
│   │   └── PsiPartition.sh
│   └── protein/
│       ├── extract.py
│       ├── hPartition.sh
│       ├── mPartition.sh
│       └── PsiPartition.sh
└── simulated/
    ├── gPartition.sh
    ├── hPartition.sh
    ├── plot.py
    ├── PsiPartition.sh
    └── summarize.py
```

## General Script Flow

Most of the scripts in this benchmark suite follow a two-step process:
1.  **Partitioning:** Run the respective partitioning algorithm (e.g., `gPartition`, `hPartition`, `mPartition`) on the input alignments to generate a partition scheme.
2.  **Evaluation:** Evaluate the resulting partition models by running a consistent IQ-TREE 2.4.0 command to infer the phylogenetic trees (`iqtree2 -s <alignment> -p <partition_file> -pre <output> -redo`).

**Note:** `PsiPartition` is an exception to this flow. It performs both partitioning and evaluation iteratively within its own Bayesian optimization run (`PsiPartition_wandb.py`), so a separate IQ-TREE evaluation step is not required in its scripts.

## 1. Empirical Data Scripts (`empricial/`)

The `empricial` folder contains scripts for benchmarking on real-world DNA and Protein alignments. These scripts generate PBS cluster jobs (`.pbs` files in `../pbs_scripts`) to partition the alignments and run phylogenetic inference using `iqtree2`.

### 1.1 DNA Alignments (`empricial/dna/`)

These scripts iterate over all `.phy` files in `../test/` and submit jobs to partition the data and infer trees.

*   **`gPartition.sh`**: Runs `gPartition.py` on the DNA alignments and subsequently evaluates the resulting partition models using IQ-TREE 2 (`iqtree2`).
*   **`hPartition.sh`**: Runs our fast partitioning method `hPartition` built directly into IQ-TREE 2 (`iqtree2-mpi -hPartition`). It restricts the substitution model set to `JC69,F81,HKY,GTR` for DNA.
*   **`hPartition-PF.sh`**: Runs `hPartition-PF`, an extended version of `hPartition` that incorporates PartitionFinder (`-hPartition-pf`) to merge subsets for a better model fit.
*   **`PsiPartition.sh`**: Runs `PsiPartition_wandb.py` on the DNA alignments. It uses Bayesian optimization (`--n_iter 30`) and allows up to 30 partitions (`--max_partitions 30`).

### 1.2 Protein/Amino Acid Alignments (`empricial/protein/`)

These scripts benchmark the partitioning methods on protein datasets located in `../test/AA/`.

*   **`hPartition.sh`**: Runs `hPartition` on the protein alignments using IQ-TREE 2.
*   **`mPartition.sh`**: Runs `mPartition.py` on the protein alignments and evaluates the resulting partition blocks with IQ-TREE 2. Note: `gPartition` is not included here because it currently does not support protein data.
*   **`PsiPartition.sh`**: Runs `PsiPartition_wandb.py` for amino acid sequences (`--alphabet aa`).
*   **`extract.py`**: A Python utility script to extract a random subset of `k` `CHARSET`s from a large NEXUS alignment file to generate smaller datasets (5, 10, or 20 loci combinations). It outputs a new concatenated alignment in PHYLIP format, along with its associated partition scheme (`.nex`).

## 2. Simulated Data Scripts (`simulated/`)

The `simulated` folder contains scripts used to evaluate the topological accuracy (Robinson-Foulds distance) of the inferred phylogenetic trees against the true simulated trees. These scripts process alignments located in `../test/simulated/`.

*   **`gPartition.sh`**, **`hPartition.sh`**, and **`PsiPartition.sh`**: Similar to the empirical scripts, these generate and submit PBS jobs to reconstruct phylogenetic trees from simulated datasets across varying numbers of loci. 
*   **`plot.py`**: A Python script that uses `matplotlib` to plot the average Robinson-Foulds distances across different numbers of loci (1 to 10), providing a visual comparison of tree reconstruction accuracy between `PsiPartition`, `gPartition`, and `hPartition`.
*   **`summarize.py`**: A Python script utilizing the `ete3` library to calculate the Robinson-Foulds (RF) distances between the true trees (e.g., `../test/simulated/loci_X/Y.tree`) and the inferred trees from the partitioned models. It averages the RF and normalized RF distances across replicates for each locus limit.