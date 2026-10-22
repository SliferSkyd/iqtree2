"""Figures 3 and 4: the distribution of RF distances at each locus count.

Each box covers the replicate alignments at one locus count (median, interquartile range,
whiskers at 1.5 x IQR). Means are deliberately not plotted: a mean cannot show whether a
method's advantage is consistent across replicates or produced by a few alignments, which is
what these figures are for.

Input is the long-format CSV written by summarize.py, so the figure is always drawn from the
measured trees and never from values pasted into this file.

    python3 summarize.py > per_rep.csv
    python3 plot.py per_rep.csv --out figures/

With --paired, the same data is also reduced to the per-alignment comparison reported in the
Results: every method analyses the same alignments, so each competitor can be compared with
hPartition alignment by alignment and tested with a two-sided Wilcoxon signed-rank test over
the non-tied pairs. SciPy is needed for the test; without it the counts are still printed.
"""
import argparse
import collections
import csv
import re
import sys

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import Patch

METHODS = ['hPartition', 'gPartition', 'PsiPartition']
COLOURS = dict(zip(METHODS, ['#2a78d6', '#eb6834', '#1baf7a']))
INK, INK2, GRID = '#1f1f1e', '#5f5e5a', '#e4e3df'
plt.rcParams.update({'font.size': 9, 'font.family': 'DejaVu Sans', 'text.color': INK,
                     'axes.labelcolor': INK, 'xtick.color': INK2, 'ytick.color': INK2,
                     'axes.edgecolor': INK2, 'axes.linewidth': 0.8})


def read(path):
    """-> (RF values keyed by (category, method), RF keyed by (category, rep) -> method)."""
    values = collections.defaultdict(list)
    paired = collections.defaultdict(dict)
    with open(path) as handle:
        for row in csv.DictReader(handle):
            if row['method'] not in METHODS:
                continue
            values[(row['category'], row['method'])].append(int(row['RF']))
            paired[(row['category'], row['rep'])][row['method']] = int(row['RF'])
    if not values:
        sys.exit(f'no usable rows in {path}')
    return values, paired


def order(categories):
    def key(c):
        m = re.search(r'(\d+)$', c)
        return (re.sub(r'\d+$', '', c), int(m.group(1)) if m else 0)
    return sorted(categories, key=key)


def group(categories):
    """Split the categories into one figure per alignment family (loci_*, len_*, ...)."""
    families = collections.defaultdict(list)
    for c in order(categories):
        families[re.sub(r'\d+$', '', c).rstrip('_')].append(c)
    return families


def style(bp, colour):
    for box in bp['boxes']:
        box.set(facecolor=colour + '55', edgecolor=colour, linewidth=1.1)
    for part in bp['whiskers'] + bp['caps']:
        part.set(color=colour, linewidth=1.0)
    for median in bp['medians']:
        median.set(color=INK, linewidth=1.6)


def figure(values, categories, path):
    width = 0.26
    fig, ax = plt.subplots(figsize=(11.0, 5.0))
    fig.patch.set_facecolor('white')
    present = [m for m in METHODS if any((c, m) in values for c in categories)]
    for index, method in enumerate(present):
        offset = (index - (len(present) - 1) / 2) * width
        series = [values.get((c, method), []) for c in categories]
        if not any(series):
            continue
        bp = ax.boxplot(series, positions=[i + offset for i in range(len(categories))],
                        widths=width * 0.86, patch_artist=True, showfliers=False,
                        showcaps=True, whis=1.5, zorder=3)
        style(bp, COLOURS[method])
    ax.set_xticks(range(len(categories)))
    ax.set_xticklabels([re.search(r'(\d+)$', c).group(1) for c in categories])
    ax.set_xlim(-0.6, len(categories) - 0.4)
    ax.set_ylabel('RF distance to true tree', fontsize=15)
    ax.set_xlabel('number of loci', fontsize=15)
    ax.tick_params(axis='both', labelsize=13)
    ax.yaxis.grid(True, color=GRID, linewidth=0.8)
    ax.set_axisbelow(True)
    for side in ('top', 'right'):
        ax.spines[side].set_visible(False)
    handles = [Patch(facecolor=COLOURS[m] + '55', edgecolor=COLOURS[m], linewidth=1.6, label=m)
               for m in present]
    ax.legend(handles=handles, loc='upper left', frameon=False, ncol=len(present), fontsize=16,
              handlelength=2.2, handleheight=1.5, columnspacing=2.4, borderaxespad=0.2,
              handletextpad=0.7)
    top = max(max(v) for k, v in values.items() if k[0] in categories and v)
    ax.set_ylim(bottom=-1, top=top * 1.16)
    fig.tight_layout()
    fig.savefig(path, dpi=300, facecolor='white')
    print('wrote', path, file=sys.stderr)


def paired_test(paired, categories, label):
    try:
        from scipy.stats import wilcoxon
    except ImportError:
        wilcoxon = None
        print('scipy not installed; reporting counts without p-values', file=sys.stderr)
    print(f'\n{label}: paired per-alignment comparison against hPartition')
    for method in METHODS[1:]:
        pairs = [(p['hPartition'], p[method]) for (c, _), p in paired.items()
                 if c in categories and 'hPartition' in p and method in p]
        if not pairs:
            continue
        better = sum(1 for h, o in pairs if h < o)
        worse = sum(1 for h, o in pairs if h > o)
        tied = sum(1 for h, o in pairs if h == o)
        line = (f'  vs {method:<13} {len(pairs):>4} alignments: '
                f'{better} better, {worse} worse, {tied} tied')
        differences = [o - h for h, o in pairs if h != o]
        if wilcoxon and differences:
            line += f'   p = {wilcoxon(differences).pvalue:.2g}'
        print(line)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('csv', nargs='?', default='per_rep.csv',
                    help='long-format CSV from summarize.py')
    ap.add_argument('--out', default='.', help='directory to write the figures into')
    ap.add_argument('--paired', action='store_true',
                    help='also print the paired comparison and Wilcoxon tests')
    args = ap.parse_args()

    values, paired = read(args.csv)
    families = group({c for c, _ in values})
    for name, categories in families.items():
        figure(values, categories, f'{args.out.rstrip("/")}/rf-{name}.png')
        if args.paired:
            paired_test(paired, set(categories), name)


if __name__ == '__main__':
    main()
