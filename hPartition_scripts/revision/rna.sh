#!/bin/bash
# Benchmark on the four multi-locus alignments containing structural RNA (Tables 4, 6 and
# Supplementary Table S6). Same protocol as the empirical DNA benchmark: each method builds a
# scheme, then stock IQ-TREE 2.4.0 scores it from scratch at seed 0.
#
# PsiPartition is the exception, as everywhere else: it scores candidate schemes inside its own
# Bayesian optimisation loop, so its criterion comes from its own final evaluation.
#
# This and the other scripts in revision/ ran on the replacement cluster under Slurm; the
# original benchmark in ../empirical and ../simulated ran under PBS on hardware that was
# decommissioned before the revision. See the Experimental Setups section of the paper.

set -eu

ALN=${ALN:-../test/rna}                                  # <dataset>.phy and <dataset>.fasta
OUT=${OUT:-../output/rna}
HPAR=$(readlink -f "${HPAR:-../build/iqtree2}")          # the hPartition fork
IQ=$(command -v "${IQ:-iqtree2}" || readlink -f "${IQ:-iqtree2}")   # stock IQ-TREE 2.4.0
GDIR=${GDIR:-../gPartition}
PSIDIR=${PSIDIR:-../PsiPartition}
PARTITION=${PARTITION:-normal}

DATASETS=${DATASETS:-"barkbeetles midges hummingbirds saxifragales"}

submit() {   # submit <name> <body>
  sbatch --job-name="$1" --partition="$PARTITION" \
         --ntasks=1 --cpus-per-task=1 --mem=10G \
         --output="$OUT/logs/$1.out" --wrap="$2"
}

mkdir -p "$OUT/logs"

for ds in $DATASETS; do
  # hPartition
  o1="$OUT/hPartition/$ds/hPartition"; o2="$OUT/hPartition/$ds/partition"
  mkdir -p "$o1" "$o2"
  submit "rna_${ds}_hPar" "
    export OMP_NUM_THREADS=1
    $HPAR -seed 0 -s $ALN/$ds.phy -pre $o1/aln -hPartition -redo \
          -mset JC69,F81,HKY,GTR -T 1 > $o1/cmd.log 2>&1
    $IQ -seed 0 -s $ALN/$ds.phy -p $o1/aln.partitions -pre $o2/aln -redo -T 1 > $o2/cmd.log 2>&1"

  # gPartition
  o1="$OUT/gPartition/$ds/gPartition"; o2="$OUT/gPartition/$ds/partition"
  mkdir -p "$o1" "$o2"
  submit "rna_${ds}_gPar" "
    export OMP_NUM_THREADS=1
    cd $GDIR && python3 gPartition.py -f $ALN/$ds.phy -o $o1/ > $o1/cmd.log 2>&1
    $IQ -seed 0 -s $ALN/$ds.phy -p $o1/$ds.phy.FINAL.nex -pre $o2/aln -redo -T 1 > $o2/cmd.log 2>&1"

  # PsiPartition: partitions and evaluates in one run
  o="$OUT/PsiPartition/$ds"
  mkdir -p "$o"
  submit "rna_${ds}_PsiPar" "
    cd $PSIDIR && python3 PsiPartition_wandb.py --msa $ALN/$ds.fasta --format fasta \
         --alphabet dna --max_partitions 30 --n_iter 30 > $o/cmd.log 2>&1"
done

echo "submitted $(echo $DATASETS | wc -w) datasets x 3 methods"
