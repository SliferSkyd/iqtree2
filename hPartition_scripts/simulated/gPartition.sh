#!/bin/bash

# Directory where the PBS scripts will be generated
output_script_dir="../pbs_scripts"
mkdir -p "$output_script_dir"

# Loop through all .phy files in ../test/simulated/<folder>
for file in ../test/simulated/*/*.phy; do
  base_name=$(basename "$file" .phy)
  folder_name=$(basename $(dirname "$file"))
  test_dir=$(dirname "$file")

  # Reference tree is in test folder
  ref_tree="${test_dir}/${base_name}.tree"

  # Define output directories
  output_dir1="../output/test-simulated-dna-gPar/${folder_name}/${base_name}/gPartition/"
  output_dir2="../output/test-simulated-dna-gPar/${folder_name}/${base_name}/partition/"
 
  partition_dir="${output_dir1}/${base_name}.phy.FINAL.nex"

  # Use inferred tree from partitioned model for comparison
  inf_tree="${output_dir2}.treefile"

  mkdir -p "$output_dir1" "$output_dir2"

  # PBS script path
  script_path="$output_script_dir/${folder_name}_${base_name}_job.pbs"
  cat << EOF > "$script_path"
#!/bin/bash
#PBS -N ${folder_name}_${base_name}_gPar
#PBS -j oe
#PBS -m abe
#PBS -l select=1:ncpus=1:ompthreads=1:mem=10G:mpiprocs=1
#PBS -q para_cpu

cd \$PBS_O_WORKDIR
module load mpi/openmpi-x86_64
export OMP_NUM_THREADS=1

cd ../gPartition
source deactivate
python --version

python gPartition.py \\
  -f "$file" \\
  -o "$output_dir1" \\
  > "$output_dir1/cmd.log"

/datausers/ioit/thaontp/danquan/partitionAnalysis/iqtree-2.4.0-Linux-intel/bin/iqtree2 \\
  -seed 0 \\
  -s "$file" \\
  -p "$partition_dir" \\
  -pre "$output_dir2" \\
  -redo \\
  > "$output_dir2/cmd.log"

EOF

  echo "Generated PBS script for $file at $script_path"
  qsub "$script_path"
done
