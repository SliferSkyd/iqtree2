#!/bin/bash

# Directory where the PBS scripts will be generated
output_script_dir="../pbs_scripts"
mkdir -p "$output_script_dir"


# Loop through all .phy files in ../test/
for file in ../test/*.phy; do
# Extract the base filename without the extension
  base_name=$(basename "$file" .phy)

  # Define the output directory for each file
  output_dir1="../output/real/gPar/$base_name/"
  output_dir2="../output/real/gPar-score/$base_name/"
  partition_dir="../output/real/gPar/$base_name/${base_name}.phy.FINAL.nex"

  mkdir -p "$output_dir1"
  mkdir -p "$output_dir2"
  

  # Create a PBS script for each .phy file
  script_path="$output_script_dir/${base_name}_job.pbs"
  cat << EOF > "$script_path"
#!/bin/bash
#PBS -N ${base_name}_gPar
#PBS -j oe
#PBS -m abe
#PBS -l select=1:ncpus=1:ompthreads=1:mem=10G:mpiprocs=1
#PBS -q para_cpu

cd \$PBS_O_WORKDIR
module load mpi/openmpi-x86_64
module avail
module list
export OMP_NUM_THREADS=1

cd \$PBS_O_WORKDIR
cd ../gPartition
source deactivate
python --version

python gPartition.py \\
  -f ../test/${base_name}.phy \\
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
  qsub $script_path


done
