from ete3 import Tree
import os

for loci in range(1, 11):
    total_rf = 0
    total_norm_rf = 0
    count = 0

    for rep in range(10):
        true_path = f"../test/simulated/loci_{loci}/{rep}.tree"
        inferred_path = f"../output/test-simulated-dna/loci_{loci}/{rep}/partition/.treefile"
    
        if os.path.exists(true_path) and os.path.exists(inferred_path):
            try:
                t1 = Tree(true_path)
                t2 = Tree(inferred_path)
                
                rf, max_rf, _, _, _, _, _ = t1.robinson_foulds(t2, unrooted_trees=True)
                norm_rf = rf / max_rf if max_rf > 0 else 0

                total_rf += rf
                total_norm_rf += norm_rf
                count += 1

            except Exception as e:
                print(f"Error at loci_{loci}/{rep}: {e}")
        else:
            print(f"Missing files at loci_{loci}/{rep}")

    if count > 0:
        avg_rf = total_rf / count
        avg_norm_rf = total_norm_rf / count
        print(f"loci_{loci},{avg_rf:.4f},{avg_norm_rf:.4f}\n")
    else:
        print(f"loci_{loci},NA,NA\n")
