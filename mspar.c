#define _GNU_SOURCE

const int SEEDS_COUNT = 3;
const int SEED_TAG = 100;
const int SAMPLES_NUMBER_TAG = 200;

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "ms.h"
#include "mspar.h"
#include <mpi.h> /* OpenMPI library */

/*
 * Función que dada una lista workers, devuelve el índice de esta lista que corresponde a
 * un worker ocioso.
 *
 * @param workersActivity estado de actividad de los workers (0=ocioso; 1=ocupado)
 * @param poolSize el largo de la lista de workers
 * @lastAssignedWorker el índice del último worker al que se le asignó tareas
 *
 * @return en caso de encontrar un worker ocioso, se devuelve su índice de la lista de workers.
 *         En caso contrario se devuelve -1, lo cual significa que todos los workers están ocupados.
 */
int findIdleWorker(int* workersActivity, int poolSize, int lastAssignedWorker) {
  /*
   * Nota, el valor de lastIdleWorker se utiliza para dar oportunidad de ocupación a todos los
   * workers. De otra forma habría que siempre comenzar desde 0, lo cual puede implicar que
   * solamente los workers con menor índice sean los que siempre estén ocupados.
   */

  int result = -1;
  int i=lastAssignedWorker+1;
  while(i < poolSize && workersActivity[i] == 1){
    i++;
  };

  if(i >= poolSize){
    i=1; // El proceso 0 es el master, por lo que no se cuenta.
    while(i < lastAssignedWorker && workersActivity[i]==1){
      i++;
    }

    if(i <= lastAssignedWorker && workersActivity[i]==0){
      result = i;
    }
  } else {
    result = i;
  }

  return result;
}

/*
 * Assigns samples to the workers. This implies to send the number of samples to be generated..
 *
 * @param workersActivity worker's state (0=idle; 1=busy)
 * @param worker worker's index to whom a sample is going to be assigned
 * @param samples samples the worker is going to generate
 */
void assignWork(int* workersActivity, int worker, int samples) {
  MPI_Send(&samples, 1, MPI_INT, worker, SAMPLES_NUMBER_TAG, MPI_COMM_WORLD);
 //TODO check usage of MPI_Sendv??
  workersActivity[worker]=1;
}

/*--------------------------------------------------------------
 *
 *  DESCRIPTION: (Append strings)  CMS
 *
 *    Given two strings, lhs and rhs, the rhs string is appended
 *    to the lhs string, which can later on can be safely accessed
 *    by the caller of this function.
 *
 *  ARGUMENTS:
 *
 *    lhs - The left hand side string
 *    rhs - The right hand side string
 *
 *  RETURNS:
 *    A pointer to the new string (rhs appended to lhs)
 *
 *------------------------------------------------------------*/
char *
append(char *lhs, const char *rhs)
{
	size_t len1 = strlen(lhs);
	size_t len2 = strlen(rhs) + 1; //+1 because of the terminating null

	lhs = realloc(lhs, len1 + len2);

	return strncat(lhs, rhs, len2);
} /* append */

/*
 * Receives the sample's quantity the Master process asked to be generated.
 *
 * @return samples to be generated
 */
int receiveWorkRequest(){
  int samples;
  MPI_Status status;

  MPI_Recv(&samples, 1, MPI_INT, 0, SAMPLES_NUMBER_TAG, MPI_COMM_WORLD, &status);
  return samples;
}

// **************************************  //
// WORKERS
// **************************************  //

/*
 * Prints the number of segregation sites:
 *    \n
 *    // xxx.x xx.xx x.xxxx x.xxxx
 *    segsites: xxx
 */
char *doPrintWorkerResultHeader(int segsites, double probss, struct params pars){
    char *append(char *lhs, const char *rhs);
    int i;
    char *tempString;

    char *results = malloc(3);
    sprintf(results, "\n//");

    if( (segsites > 0 ) || ( pars.mp.theta > 0.0 ) ) {
        if( (pars.mp.segsitesin > 0 ) && ( pars.mp.theta > 0.0 )) {
            asprintf(&tempString, "prob: %g\n", probss);
            results = append(results, tempString);
        }
        asprintf(&tempString, "\nsegsites: %d\n",segsites);
        results = append(results, tempString);
    }

    return results;
}

/*
 * Prints the segregation site positions:
 *      positions: 0.xxxxx 0.xxxxx .... etc.
 */
char *doPrintWorkerResultPositions(int segsites, int output_precision, double *posit, char *results){
    char *append(char *lhs, const char *rhs);
    int i;
    char tempString[3 + output_precision]; //number+decimal point+space

    results = append(results, "positions: ");

    for(i=0; i<segsites; i++){
        sprintf(tempString, "%6.*lf ", output_precision, posit[i]);
        results = append(results, tempString);
    }
    return append(results, tempString);
}

/*
 * Prints the gametes
 */
char *doPrintWorkerResultGametes(int segsites, int nsam, char **gametes, char *results){
    char *append(char *lhs, const char *rhs);
    int i;
    char tempString[segsites+1]; //segsites + LF/CR

    results = append(results, "\n");

    for(i=0;i<nsam; i++) {
        sprintf(tempString, "%s\n", gametes[i]);
        results = append(results, tempString);
    }

    return results;
}

/* Initialization of the random generator. */
unsigned short * parallelSeed(unsigned short *seedv){
  unsigned short *seed48();

  return seed48(seedv);
}
