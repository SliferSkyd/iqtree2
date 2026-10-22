#!/bin/bash
# Supplementary Table S4: does the candidate model set explain hPartition's advantage?
#
# hPartition is run throughout the paper with -mset JC69,F81,HKY,GTR, while gPartition selects
# models with -m MFP over IQ-TREE's full default set. A referee could reasonably ask whether the
# difference in fit follows from that rather than from the partitioning. This script removes the
# difference: hPartition is rerun with -mset omitted, so both methods search the same set, and
# gPartition is rerun beside it on the same node.
#
# The evaluation step never used -mset, so the scores stay comparable with the main tables.

set -eu

ALN=${ALN:-../test}
OUT=${OUT:-../output/matched-model-set}
HPAR=$(readlink -f "${HPAR:-../build/iqtree2}")
IQ=$(command -v "${IQ:-iqtree2}" || readlink -f "${IQ:-iqtree2}")
GDIR=${GDIR:-../gPartition}
PARTITION=${PARTITION:-normal}

submit() {
  sbatch --job-name="$1" --partition="$PARTITION" \
         --ntasks=1 --cpus-per-task=1 --mem=10G \
         --output="$OUT/logs/$1.out" --wrap="$2"
}

mkdir -p "$OUT/logs"

for file in "$ALN"/*.phy; do
  ds=$(basename "$file" .phy)

  # hPartition with no -mset: its internal search uses ModelFinder's full default DNA set
  o1="$OUT/hPartition/$ds/hPartition"; o2="$OUT/hPartition/$ds/partition"
  mkdir -p "$o1" "$o2"
  submit "nomset_${ds}_hPar" "
    export OMP_NUM_THREADS=1
    $HPAR -seed 0 -s $file -pre $o1/aln -hPartition -redo -T 1 > $o1/cmd.log 2>&1
    $IQ -seed 0 -s $file -p $o1/aln.partitions -pre $o2/aln -redo -T 1 > $o2/cmd.log 2>&1"

  # gPartition on the same node, for a like-for-like comparison
  o1="$OUT/gPartition/$ds/gPartition"; o2="$OUT/gPartition/$ds/partition"
  mkdir -p "$o1" "$o2"
  submit "nomset_${ds}_gPar" "
    export OMP_NUM_THREADS=1
    cd $GDIR && python3 gPartition.py -f $file -o $o1/ > $o1/cmd.log 2>&1
    $IQ -seed 0 -s $file -p $o1/$ds.phy.FINAL.nex -pre $o2/aln -redo -T 1 > $o2/cmd.log 2>&1"
done
