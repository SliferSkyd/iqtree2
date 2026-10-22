#!/bin/bash
# hPartition on binary data.
#
# The pipeline reuses IQ-TREE's alignment and state-space infrastructure, so it is not
# restricted to four states. This checks that claim on binary data rather than asserting it:
# a DNA alignment is RY-recoded (purines -> 0, pyrimidines -> 1) and run with -st BIN.
#
# Codon and morphological data are not tested here, and the paper does not claim they are.

set -eu

ALN=${ALN:-../test}
OUT=${OUT:-../output/binary}
HPAR=$(readlink -f "${HPAR:-../build/iqtree2}")
IQ=$(command -v "${IQ:-iqtree2}" || readlink -f "${IQ:-iqtree2}")
PARTITION=${PARTITION:-normal}
DATASET=${DATASET:-Morpho}

mkdir -p "$OUT/logs"

# RY-recode: A,G -> 0; C,T,U -> 1; everything else (gaps, ambiguity) -> -
python3 - "$ALN/$DATASET.phy" "$OUT/$DATASET.ry.phy" <<'PY'
import sys

src, dst = sys.argv[1], sys.argv[2]
table = str.maketrans({c: v for c, v in
                       list(zip('AGag', '0000')) + list(zip('CTUctu', '111111'))})
with open(src) as fin, open(dst, 'w') as fout:
    fout.write(fin.readline())                      # the PHYLIP header is copied unchanged
    for line in fin:
        parts = line.split(None, 1)                 # name and sequence; the separator is a tab
        if len(parts) != 2:                         # in these files, not a space
            fout.write(line)
            continue
        name, seq = parts
        seq = ''.join(c if c in '01' else '-' for c in seq.strip().translate(table))
        fout.write(f'{name}\t{seq}\n')
print('wrote', dst)
PY

o1="$OUT/$DATASET/hPartition"; o2="$OUT/$DATASET/partition"
mkdir -p "$o1" "$o2"

sbatch --job-name="bin_$DATASET" --partition="$PARTITION" \
       --ntasks=1 --cpus-per-task=1 --mem=10G \
       --output="$OUT/logs/bin_$DATASET.out" --wrap="
  export OMP_NUM_THREADS=1
  $HPAR -seed 0 -s $OUT/$DATASET.ry.phy -st BIN -pre $o1/aln -hPartition -redo -T 1 \
        > $o1/cmd.log 2>&1
  $IQ -seed 0 -s $OUT/$DATASET.ry.phy -st BIN -p $o1/aln.partitions -pre $o2/aln -redo -T 1 \
        > $o2/cmd.log 2>&1"
