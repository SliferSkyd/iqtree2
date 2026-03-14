#!/bin/bash

# Directory where the PBS scripts will be generated
output_script_dir="../pbs_scripts"
mkdir -p "$output_script_dir"

# Loop through all .fasta files in ../test/simulated/<folder>
for file in ../test/simulated/*/*.fasta; do
  base_name=$(basename "$file" .fasta)
  folder_name=$(basename $(dirname "$file"))
  test_dir=$(dirname "$file")

  # PBS script path
  script_path="$output_script_dir/${folder_name}_${base_name}_job.pbs"
  cat << EOF > "$script_path"
#!/bin/bash
#PBS -N ${folder_name}_${base_name}_PsiPar
#PBS -j oe
#PBS -m abe
#PBS -l select=1:ncpus=1:ompthreads=1:mem=10G:mpiprocs=1
#PBS -q para_cpu

cd \$PBS_O_WORKDIR
cd ../PsiPartition
conda activate PsiPartition

python3 PsiPartition_wandb.py --msa $file --format fasta --alphabet dna --max_partitions 30 --n_iter 30

EOF

  echo "Generated PBS script for $file at $script_path"
  qsub "$script_path"
done
