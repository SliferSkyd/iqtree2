This release introduces **hPartition**, a novel computational partitioning framework that simultaneously optimizes computational efficiency and model fit for genome-scale phylogenetic inference. Fully integrated into IQ-TREE 2, hPartition is capable of efficiently analyzing massive alignments involving millions of sites accurately and at a fraction of the time required by existing wrapper-based methods.

### ✨ Key Features
* **Native IQ-TREE 2 Integration:** Implemented directly in C++ within the IQ-TREE 2 engine for significantly improved memory usage, tight integration with core algorithms, and unparalleled computational speed.
* **Hybrid Partitioning Strategy:** Incorporates fast, model-free evolutionary rate estimation via `fastTIGER`, followed by seed partitioning, substitution model unifying, and probabilistic site repartitioning/cleanup.
* **Broad Data Support:** Fully applicable to both large-scale DNA and protein/amino acid alignments.
* **Invariant Site Handling:** Distributes invariant sites probabilistically across partitions based on best-fit rather than grouping them into a single cluster, resolving biases that plagued earlier partitioning methods.
* **hPartition-PF:** Includes an extended variant (`-hPartition-pf`) that incorporates PartitionFinder's heuristic search strategy to merge resulting subsets, yielding highly parsimonious schemes with better model fit.

### 📊 Performance Highlights
* **Speed:** Completes partitioning tasks drastically faster than `gPartition`, `mPartition`, and `PsiPartition`.
* **Accuracy:** Consistently achieves better BIC/AICc scores on empirical datasets, and reconstructs more accurate phylogenies (lowest Robinson-Foulds distances) on simulated datasets.
* **Scalability:** Easily handles massive alignments with up to hundreds of thousands of sites where other memory-intensive methods fail.

### 🧪 Benchmark Scripts & Reproducibility  
The scripts used to perform the experiments on both empirical and simulated datasets, along with instructions on how to replicate the benchmarking process, are included in this repository. 
Please see the [hPartition_scripts](cci:7://file:///Users/hoangvu/iqtree2/hPartition_scripts:0:0-0:0) directory and refer to the [hPartition_scripts/README.md](cci:7://file:///Users/hoangvu/iqtree2/hPartition_scripts/README.md:0:0-0:0) for a comprehensive guide on the script workflows, execution, and evaluation procedures.

### 🚀 Usage

To partition your alignment using the ultra-fast standard **hPartition**, run:

```bash
iqtree2 -s ALIGNMENT_FILE -mset JC69,F81,HKY,GTR -hPartition # For DNA datasets
iqtree2 -s ALIGNMENT_FILE -hPartition # For Protein datasets
```
To use the extended **hPartition-PF**, simply append `-pf`:
```bash
iqtree2 -s ALIGNMENT_FILE -mset JC69,F81,HKY,GTR -hPartition-pf # For DNA datasets
```