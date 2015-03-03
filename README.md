mspar [![Build Status](https://travis-ci.org/cmontemuino/mspar.svg?branch=master)](https://travis-ci.org/cmontemuino/mspar)
=====

Parallel version of "ms" coalescent simulator using a master worker approach and a MPI implementation with on-demand scheduling.

## Usage ##
Make the program:
`$ make clean bin/mspar`

Example running with 2 processes:
`$ mpirun -n 2 mspar 10 10 -seeds 12343 2334 1112 -t 100 -r 100 1000 > samples.out`

Example running with 2 processes and tbs arguments:
`$ mpirun -n 2 mspar 10 10 -seeds 12343 2334 1112 -t tbs -r tbs 1000 < priors.txt > samples.out`


## Known Issues / Limitations ##
* Seeds must be specified in the command line (constraint of the parallel implementation)
