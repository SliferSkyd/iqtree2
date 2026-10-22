#!/bin/bash

# Paths to the three binaries used below. Override them in the environment rather than
# editing this file:  HPAR=/path/to/iqtree2 ./mPartition.sh
HPAR=$(readlink -f "${HPAR:-../build/iqtree2}")                 # the hPartition fork of IQ-TREE 2
IQ=$(command -v "${IQ:-iqtree2}" || readlink -f "${IQ:-iqtree2}")                              # stock IQ-TREE 2.4.0, used for evaluation only
PYTHON=${PYTHON:-python3}

# Directory where the PBS scripts will be generated
output_script_dir="../pbs_scripts"
mkdir -p "$output_script_dir"


# Loop through all .phy files in ../test/AA/
for file in ../test/AA/*; do
# Extract the base filename without the extension
  base_name=$(basename "$file" .phy)

  # Define the output directory for each file
  output_dir1="../output/AA/mPar/$base_name/"
  output_dir2="../output/AA/mPar-score/$base_name/"
  partition_dir="../output/AA/mPar/$base_name/Results/par.${base_name}"

  mkdir -p "$output_dir1"
  mkdir -p "$output_dir2"
  

  # Create a PBS script for each .phy file
  script_path="$output_script_dir/${base_name}_job.pbs"
  cat << EOF > "$script_path"
#!/bin/bash
#PBS -N ${base_name}_mPar
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
cd ../mPartition
conda deactivate
"$PYTHON" --version

"$PYTHON" mPartition.py \\
  -f ../test/AA/${base_name} \\
  -o "$output_dir1" \\
  > "$output_dir1/cmd.log"

"$IQ" \\
  -seed 0 \\
  -s "$file" \\
  -p "$partition_dir" \\
  -pre "$output_dir2" \\
  -redo \\
  > "$output_dir2/cmd.log"  

EOF

  echo "Generated PBS script for $file at $script_path"
  #qsub $script_path


done
