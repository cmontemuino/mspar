#!/bin/bash -l

# Expected parameters: -N: nodes; -n: mpi procs
#SBATCH --get-user-env
#SBATCH --exclusive
#SBATCH --distribution=block:cyclic
#SBATCH --output=%j.out
#SBATCH --error=%j.err
#SBATCH --workdir=/home/cmontemu/projects/experimentacion/data

# Load OpenMPI libraries
module load openmpi/1.6.5

# mspar executable
MSPAR=/home/cmontemu/projects/mspar/bin/mspar

# Run mspar
/usr/bin/time -v -o $SLURM_JOB_NAME.$SLURM_JOB_ID.time mpirun -n $SLURM_NTASKS $MSPAR 100 200 -seeds 63867 8507 42806 -t 1000 -r 100 10000
