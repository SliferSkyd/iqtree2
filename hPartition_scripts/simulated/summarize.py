"""Robinson-Foulds distances on the simulated datasets, one row per alignment.

Writes a long-format CSV (category, replicate, method, RF, normalised RF). Per-replicate rows
are what plot.py and the paired tests are built from; averaging here would discard the
variability the figures are meant to show, so nothing is averaged unless --summary is passed.

Directory layout, as produced by the job scripts in this directory:

    <out>/test-simulated-dna/<category>/<rep>/partition/.treefile        hPartition
    <out>/test-simulated-dna-gPar/<category>/<rep>/partition/.treefile   gPartition
    <out>/test-simulated-dna-psi/<category>/<rep>/final.nwk              PsiPartition
    <true>/<category>/<rep>.tree                                         true tree

<category> is whatever the alignment directories are named (loci_1 ... loci_10 for SimDNA1,
len_6 ... len_15 for SimDNA2); they are discovered rather than assumed. Override a method's
directory with --dir METHOD=PATH.

Usage:
    python3 summarize.py > per_rep.csv
    python3 summarize.py --summary > per_rep.csv
"""
import argparse
import csv
import os
import re
import sys

from ete3 import Tree

DIRS = {'hPartition': 'test-simulated-dna',
        'gPartition': 'test-simulated-dna-gPar',
        'PsiPartition': 'test-simulated-dna-psi'}
# PsiPartition scores schemes inside its own optimisation loop and writes final.nwk instead of
# going through a separate IQ-TREE evaluation, so more than one tree name has to be accepted.
TREE_NAMES = ['partition/.treefile', 'partition/aln.treefile', 'final.nwk']


def inferred_tree(run_dir):
    for name in TREE_NAMES:
        path = os.path.join(run_dir, name)
        if os.path.exists(path) and os.path.getsize(path):
            return path
    return None


def rf(true_path, inferred_path):
    t1, t2 = Tree(true_path), Tree(inferred_path)
    dist, max_dist = t1.robinson_foulds(t2, unrooted_trees=True)[:2]
    return dist, (dist / max_dist if max_dist else 0.0)


def sort_key(category):
    m = re.search(r'(\d+)$', category)
    return (re.sub(r'\d+$', '', category), int(m.group(1)) if m else 0)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', default='../output', help='root of the method output directories')
    ap.add_argument('--true', default='../test/simulated', help='root of the true trees')
    ap.add_argument('--dir', action='append', default=[], metavar='METHOD=PATH',
                    help='override a method output directory (repeatable)')
    ap.add_argument('--summary', action='store_true', help='print per-category means to stderr')
    args = ap.parse_args()

    dirs = dict(DIRS)
    for override in args.dir:
        method, _, path = override.partition('=')
        dirs[method] = path

    if not os.path.isdir(args.true):
        sys.exit(f'no true-tree directory at {args.true}')
    categories = sorted((d for d in os.listdir(args.true)
                         if os.path.isdir(os.path.join(args.true, d))), key=sort_key)
    if not categories:
        sys.exit(f'no alignment categories under {args.true}')

    writer = csv.writer(sys.stdout)
    writer.writerow(['category', 'rep', 'method', 'RF', 'normRF'])
    means, missing = {}, 0

    for category in categories:
        reps = sorted(int(m.group(1))
                      for m in (re.fullmatch(r'(\d+)\.tree', f)
                                for f in os.listdir(os.path.join(args.true, category))) if m)
        for method, subdir in dirs.items():
            values = []
            for rep in reps:
                true_path = os.path.join(args.true, category, f'{rep}.tree')
                run_dir = os.path.join(args.out, subdir, category, str(rep))
                inferred_path = inferred_tree(run_dir)
                if inferred_path is None:
                    print(f'missing {category} rep {rep} {method}', file=sys.stderr)
                    missing += 1
                    continue
                try:
                    dist, norm = rf(true_path, inferred_path)
                except Exception as exc:               # malformed or truncated newick
                    print(f'error {category} rep {rep} {method}: {exc}', file=sys.stderr)
                    missing += 1
                    continue
                writer.writerow([category, rep, method, dist, f'{norm:.4f}'])
                values.append(dist)
            if values:
                means[(category, method)] = sum(values) / len(values)

    if args.summary:
        methods = list(dirs)
        print('\ncategory      ' + '  '.join(f'{m:>12}' for m in methods), file=sys.stderr)
        for category in categories:
            cells = '  '.join(f'{means[(category, m)]:>12.1f}' if (category, m) in means
                              else f'{"--":>12}' for m in methods)
            print(f'{category:<13} {cells}', file=sys.stderr)
    if missing:
        print(f'\n{missing} run(s) missing or unreadable', file=sys.stderr)


if __name__ == '__main__':
    main()
