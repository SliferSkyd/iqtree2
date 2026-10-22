# hPartition Benchmark Scripts

This directory contains the scripts used to benchmark **hPartition**, **hPartition-PF**, **gPartition**, **mPartition**, and **PsiPartition** on empirical, simulated and RNA-containing datasets as described in our paper, together with the analysis scripts that turn their output into the reported figures and tables.

## Server Environment

The work spans two clusters, because the first was decommissioned between the original submission and the revision.

| | Original benchmark | Revision experiments |
|---|---|---|
| Scripts | `empirical/`, `simulated/` | `revision/` |
| CPU | Intel Xeon Gold 6132 @ 2.60 GHz, 28 cores/node | Intel Xeon E5-2697 v4 @ 2.30 GHz, 32 cores/node |
| Scheduler | PBS (`qsub`) | Slurm (`sbatch`) |

Both clusters are composed of homogeneous nodes, and on both every job of every method requested the same resources: **1 CPU core, 1 OpenMP thread, 1 MPI process and 10 GB of memory**, with random seed 0.

Wall-clock times are not comparable between the two clusters, and no table or figure in the paper mixes them. Where a revision experiment compares hPartition against a competing method, that method was rerun on the replacement cluster rather than taken from the earlier tables.

One qualification on the resource request: within that one-core allocation, hPartition, gPartition and mPartition run IQ-TREE single-threaded, whereas PsiPartition's distributed driver passes `-nt 4` to every IQ-TREE call and offers no option to change it. Its four threads share the same single core, so the allocation is equal in CPU resources but the runs are not equal in thread count. Neither gPartition nor mPartition exposes a thread option at all, so the methods were benchmarked as released rather than modified to match.

## Configuring the paths

None of the scripts hardcodes an installation path. Each reads the following from the environment, with the defaults shown:

| Variable | Default | What it is |
|---|---|---|
| `HPAR` | `../build/iqtree2` | the hPartition fork of IQ-TREE 2 built from this repository |
| `IQ` | `iqtree2` on `$PATH` | **stock** IQ-TREE 2.4.0, used only for the evaluation step |
| `PYTHON` | `python3` | interpreter for `gPartition.py` / `mPartition.py` |
| `GDIR`, `PSIDIR` | `../gPartition`, `../PsiPartition` | checkouts of the competing methods |
| `ALN`, `OUT` | see each script | input alignments and output root |

For example:

```bash
HPAR=/opt/hpartition/bin/iqtree2 IQ=/opt/iqtree-2.4.0/bin/iqtree2 ./empirical/dna/hPartition.sh
```

## Directory Structure

```
hPartition_scripts/
├── empirical/                  original benchmark, PBS
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
├── simulated/                  original benchmark, PBS
│   ├── gPartition.sh
│   ├── hPartition.sh
│   ├── PsiPartition.sh
│   ├── summarize.py            RF distances, one row per alignment
│   └── plot.py                 Figures 3 and 4, and the paired tests
└── revision/                   experiments added during revision, Slurm
    ├── rna.sh                  Tables 4, 6, S6
    ├── matched_model_set.sh    Table S4
    ├── stage2_ablation.sh      Table S5
    └── binary_data.sh          binary-data check
```

## General Script Flow

Most scripts follow a two-step process:

1. **Partitioning:** run the partitioning algorithm on the input alignment to produce a scheme.
2. **Evaluation:** score that scheme with a consistent stock IQ-TREE 2.4.0 command (`iqtree2 -seed 0 -s <alignment> -p <partition_file> -pre <output> -redo`).

The evaluation re-estimates the tree and all model parameters from scratch, so no score reported in the paper depends on whatever provisional tree a method used internally while building its scheme.

**`PsiPartition` is an exception.** It scores candidate schemes inside its own Bayesian optimisation loop (`PsiPartition_wandb.py`) and does not expose a scheme for external evaluation, so its criterion comes from its own final evaluation and no separate step is run.

## 1. Empirical Data Scripts (`empirical/`)

Scripts for the real-world DNA and protein alignments. Each generates one PBS job per alignment in `../pbs_scripts` and submits it.

### 1.1 DNA Alignments (`empirical/dna/`)

Iterate over all `.phy` files in `../test/`.

* **`hPartition.sh`** — runs `-hPartition`, restricting the candidate model set to `JC69,F81,HKY,GTR`, then evaluates.
* **`hPartition-PF.sh`** — runs `-hPartition-pf`, the optional Stage 6 merge (relaxed clustering, following the PartitionFinder logic), then evaluates. This is the comparison in Table 10.
* **`gPartition.sh`** — runs `gPartition.py`, then evaluates.
* **`PsiPartition.sh`** — runs `PsiPartition_wandb.py` with `--max_partitions 30 --n_iter 30`, the configuration distributed by its authors in `comparison/test_PsiPartition.py`.

### 1.2 Protein/Amino Acid Alignments (`empirical/protein/`)

Operate on the alignments in `../test/AA/`. `-mset` is not used here: it applies to nucleotide data only, since empirical amino-acid models differ between subsets only in their state frequencies.

* **`hPartition.sh`**, **`mPartition.sh`**, **`PsiPartition.sh`** (the last with `--alphabet aa`). `gPartition` is absent because it does not support protein data.
* **`extract.py`** — draws *k* random `CHARSET`s from a large NEXUS alignment and writes the concatenated subset in PHYLIP format with its partition scheme, which is how the 5-, 10- and 20-locus subsets were built. The draw is seeded (`--seed`, default 0); note that the exact subsets benchmarked in the paper are deposited with the data, so they need not be regenerated.

## 2. Simulated Data Scripts (`simulated/`)

Evaluate topological accuracy against the known true trees for SimDNA1 (1–10 loci) and SimDNA2 (6–15 loci), 100 alignments each.

* **`hPartition.sh`**, **`gPartition.sh`**, **`PsiPartition.sh`** — generate and submit one job per alignment. All three use the plain partitioning mode; Stage 6 is not enabled anywhere in the simulated benchmark.
* **`summarize.py`** — computes Robinson-Foulds distances between each inferred tree and its true tree with `ete3`, writing **one row per alignment** as CSV. Nothing is averaged: a mean cannot show whether a method's advantage is consistent across replicates or produced by a few alignments.
* **`plot.py`** — draws the RF **distributions** that appear as Figures 3 and 4 (box, median, interquartile range, whiskers at 1.5 × IQR), reading the CSV so that the figure is always drawn from measured trees. With `--paired` it also prints the per-alignment comparison reported in the Results: because all methods analyse the same alignments, each competitor is compared with hPartition alignment by alignment and tested with a two-sided Wilcoxon signed-rank test over the non-tied pairs.

```bash
cd simulated
python3 summarize.py --summary > per_rep.csv
python3 plot.py per_rep.csv --out . --paired
```

## 3. Revision Experiments (`revision/`)

Added in response to the reviewers; all ran on the replacement cluster under Slurm.

* **`rna.sh`** — the four multi-locus alignments containing ribosomal or transfer RNA (Tables 4 and 6; AICc in Supplementary Table S6). Note that no method compared here models the covariation between paired stem positions: IQ-TREE 2.4.0 provides no RNA secondary-structure model, so the comparison is between partitioning schemes built on site-independent models.
* **`matched_model_set.sh`** — reruns hPartition with `-mset` omitted, so that it and gPartition search the same candidate set, and reruns gPartition beside it on the same node (Supplementary Table S4). This tests whether the difference in model fit follows from the model sets rather than the partitioning.
* **`stage2_ablation.sh`** — three builds differing **only** in Stage 2, everything downstream identical, isolating the seeding step (Supplementary Table S5). It also runs the standard build at five seeds, so the ablation differences can be read against hPartition's own seed-to-seed variation. The two patched builds are separate checkouts of this repository; point `GPAR2` and `NOVAR` at their binaries.
* **`binary_data.sh`** — RY-recodes a DNA alignment and runs it with `-st BIN`, checking the claim that the pipeline is not restricted to four states. Codon and morphological data are not tested, and the paper does not claim they are.
