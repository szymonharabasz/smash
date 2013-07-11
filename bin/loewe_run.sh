#!/bin/sh
#
# sbatch ./bin/loewe_run.sh
#
# number of jobs (mulitple of 24):
# single job
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#
# job output:
#SBATCH --output=/scratch/hyihp/attems/mash.o%j
#SBATCH --error=/scratch/hyihp/attems/mash.e%j
#
# email
#SBATCH --mail-type=FAIL
#SBATCH --mail-user=attems@fias.uni-frankfurt.de
#
# job name:
#SBATCH --job-name=mash
#
# mpi stuff:
#SBATCH --partition=serial
#
# mem allocation (only 200m default)
#SBATCH --mem-per-cpu=2600
#
# default time 10min, max 8 days?:
#SBATCH --time=6-23:00:00

# output
path="/scratch/hyihp/attems/mash_a10_T12_s25_t1_${SLURM_JOB_ID}"

 # git show-ref HEAD # show sha1 tag and branch of run
git_branch=$(cat ../.git/HEAD)
git_branch=${git_branch##*: }
git_ref=$(cat ../.git/${git_branch})
echo "$git_ref $git_branch"

# output info
echo "Running on ${SLURM_NNODES} nodes."
echo "Running with ${SLURM_NTASKS} number of tasks."
echo "Executed from: ${SLURM_SUBMIT_DIR}"
echo "List of nodes: ${SLURM_JOB_NODELIST}"
echo "Job id: ${SLURM_JOB_ID}"
echo "Saving data: $path"

# Run stuff in scratch space
mkdir -p "$path"
cp params_t12_s25.txt "$path/params.txt"
cp mash "$path/"
cd $path

export OMP_NUM_THREADS=1

# start programm
srun ./mash -r ${SLURM_JOB_ID}
