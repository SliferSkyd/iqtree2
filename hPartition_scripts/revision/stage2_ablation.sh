#!/bin/bash
# Supplementary Table S5: where hPartition actually differs from gPartition.
#
# The two methods share the rate estimation, the reassignment rule, the merge threshold, the
# minimum subset size and the fixed-branch-length scoring. They differ in Stage 2. This runs
# three builds that differ ONLY in Stage 2, with everything downstream identical, so the effect
# of the seeding step is isolated:
#
#   hPartition     every site binned, invariant ones at rate 0, bin count floor(m/100)
#   gpar2          gPartition's Stage 2: variant sites only, bin count nVar / max(2T-3, 100)
#   novar          variant sites only, but hPartition's bin count floor(m/100)
#
# The third separates the two things gPartition changes together: which sites are binned, and
# how many bins there are.
#
# The two patched builds are separate checkouts of this repository with Stage 2 replaced; build
# each the usual way and point the variables below at the resulting binaries. Each build prints
# a line identifying itself at startup, so the cmd.log says which one produced a scheme.

set -eu

ALN=${ALN:-../test}
OUT=${OUT:-../output/stage2-ablation}
IQ=$(command -v "${IQ:-iqtree2}" || readlink -f "${IQ:-iqtree2}")
PARTITION=${PARTITION:-normal}

# the three builds
HPAR=$(readlink -f "${HPAR:-../build/iqtree2}")
GPAR2=$(readlink -f "${GPAR2:-../../iqtree2-gpar2/build/iqtree2}")
NOVAR=$(readlink -f "${NOVAR:-../../iqtree2-novar/build/iqtree2}")

# the eight alignments small enough to run all three builds plus five seeds each
DATASETS=${DATASETS:-"Coenonymphina Calisto Arctiina Geometridae Pieridae Choreutidae Noctuidae Morpho"}

submit() {
  sbatch --job-name="$1" --partition="$PARTITION" \
         --ntasks=1 --cpus-per-task=1 --mem=10G \
         --output="$OUT/logs/$1.out" --wrap="$2"
}

run_build() {   # run_build <label> <binary> <dataset> <seed>
  local label=$1 binary=$2 ds=$3 seed=$4
  local o1="$OUT/$label/$ds.s$seed/hPartition" o2="$OUT/$label/$ds.s$seed/partition"
  mkdir -p "$o1" "$o2"
  submit "${label}_${ds}_s${seed}" "
    export OMP_NUM_THREADS=1
    $binary -seed $seed -s $ALN/$ds.phy -pre $o1/aln -hPartition -redo \
            -mset JC69,F81,HKY,GTR -T 1 > $o1/cmd.log 2>&1
    $IQ -seed 0 -s $ALN/$ds.phy -p $o1/aln.partitions -pre $o2/aln -redo -T 1 > $o2/cmd.log 2>&1"
}

mkdir -p "$OUT/logs"

for ds in $DATASETS; do
  # the ablation itself, at seed 0
  run_build hPartition "$HPAR"  "$ds" 0
  run_build gpar2      "$GPAR2" "$ds" 0
  run_build novar      "$NOVAR" "$ds" 0

  # seed variance for the standard build, so the ablation differences can be read against it.
  # Only the partitioning seed varies; the evaluation is held at -seed 0 above, so all the
  # variation comes from Stage 4's sampling of invariant sites.
  for seed in 1 2 3 4; do
    run_build hPartition "$HPAR" "$ds" "$seed"
  done
done
