#!/usr/bin/env python3

import argparse
import random
from Bio import AlignIO

def parse_charsets_from_nexus(nexus_file):
    charsets = {}
    with open(nexus_file) as f:
        lines = f.readlines()

    for line in lines:
        if line.strip().upper().startswith("CHARSET"):
            parts = line.strip().strip(";").split()
            name = parts[1]
            start, end = map(int, parts[3].split("-"))
            charsets[name] = (start - 1, end)  # convert to 0-based
    return charsets

def extract_and_concat(aln, selected_charsets, charsets):
    result = {rec.id: "" for rec in aln}
    aln_len = aln.get_alignment_length()

    for name in selected_charsets:
        start, end = charsets[name]
        if start >= aln_len or end > aln_len:
            print(f"Warning: CHARSET '{name}' range {start+1}-{end} exceeds alignment length {aln_len}. Skipping.")
            continue
        for rec in aln:
            result[rec.id] += str(rec.seq[start:end])

    result = {k: v for k, v in result.items() if v}
    return result

def write_phylip(sequences, output_file):
    if not sequences:
        raise ValueError("No sequences to write.")
    aln_len = len(next(iter(sequences.values())))
    with open(output_file, "w") as f:
        f.write(f"{len(sequences)} {aln_len}\n")
        for taxon, seq in sequences.items():
            f.write(f"{taxon} {seq}\n")

def write_charset_file(selected, charsets, output_charset_file):
    current_pos = 1  # PHYLIP is 1-based
    with open(output_charset_file, "w") as f:
        for name in selected:
            start, end = charsets[name]
            length = end - start
            new_start = current_pos
            new_end = current_pos + length - 1
            f.write(f"CHARSET {name} = {new_start}-{new_end};\n")
            current_pos = new_end + 1


def main():
    parser = argparse.ArgumentParser(description="Extract random CHARSETs from a NEXUS alignment")
    parser.add_argument("input_nexus", help="Input .nex file with matrix and CHARSETs")
    parser.add_argument("k", type=int, help="Number of CHARSETs to select")
    parser.add_argument("output_phylip", help="Output PHYLIP file")

    args = parser.parse_args()

    # Load alignment
    aln = AlignIO.read(args.input_nexus, "nexus")

    # Parse charsets
    charsets = parse_charsets_from_nexus(args.input_nexus)
    if args.k > len(charsets):
        raise ValueError(f"Only {len(charsets)} CHARSETs available, but {args.k} requested.")

    selected = random.sample(list(charsets.keys()), args.k)
    print(f"Selected CHARSETs: {selected}")

    subset = extract_and_concat(aln, selected, charsets)
    write_phylip(subset, args.output_phylip)
    write_charset_file(selected, charsets, args.output_phylip + ".nex")

if __name__ == "__main__":
    main()