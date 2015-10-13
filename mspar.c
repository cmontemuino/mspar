#define _GNU_SOURCE

const int SEEDS_COUNT = 3;
const int SEED_TAG = 100;
const int SAMPLES_NUMBER_TAG = 200;
const int RESULTS_TAG = 300;
const int GO_TO_WORK_TAG = 400;

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "ms.h"
#include "mspar.h"
#include <mpi.h> /* OpenMPI library */

// **************************************  //
// MASTER
// **************************************  //

int
masterWorkerSetup(int argc, char *argv[], int howmany, struct params parameters)
{
    // myRank           : rank of the current process in the MPI ecosystem.
    // poolSize         : number of processes in the MPI ecosystem.
    // goToWork         : used by workers to realize if there is more work to do.
    // seedMatrix       : matrix containing the RGN seeds to be distributed to working processes.
    // localSeedMatrix  : matrix used by workers to receive RGN seeds from master.
    int myRank;
    int poolSize;
    unsigned short *seedMatrix;
    unsigned short localSeedMatrix[3];


    // MPI Initialization
    MPI_Init(&argc, &argv );
    MPI_Comm_size(MPI_COMM_WORLD, &poolSize);
    MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

    if(myRank == 0)
    {
        int i;
        // Only the master process prints out the application's parameters
        for(i=0; i<argc; i++)
        {
          fprintf(stdout, "%s ",argv[i]);
        }

        // If there are (not likely) more processes than samples, then the process pull
        // is cut up to the number of samples. */
        if(poolSize > howmany)
        {
            poolSize = howmany + 1; // the extra 1 is due to the master
        }

        int nseeds;
        doInitializeRgn(argc, argv, &nseeds, parameters);
        int dimension = nseeds * poolSize;
        seedMatrix = (unsigned short *) malloc(sizeof(unsigned short) * dimension);
        for(i=0; i<dimension;i++)
        {
          seedMatrix[i] = (unsigned short) (ran1()*100000);
        }
    }

    // Filter out workers with rank higher than howmany, meaning there are more workers than samples to be generated.
    if(myRank <= howmany)
    {
        MPI_Scatter(seedMatrix, 3, MPI_UNSIGNED_SHORT, localSeedMatrix, 3, MPI_UNSIGNED_SHORT, 0, MPI_COMM_WORLD);
        if(myRank == 0)
        {
            // Master Processing
            masterProcessingLogic(howmany, 0, poolSize);
        } else
        {
            // Worker Processing
            parallelSeed(localSeedMatrix);
        }
    }

    return myRank;
}

/*
 * Lógica de procesamiento del MASTER
 *
 * @param howmany la cantidad total de muestras a generar
 * @param lastAssignedWorker último worker al que se le asignó trabajo.
 * @param poolSize la cantidad de workers (incluido el master) que hay
 *
 */
void
masterProcessingLogic(int howmany, int lastAssignedWorker, int poolSize)
{
    int *workersActivity = (int*) malloc(poolSize * sizeof(int));
    workersActivity[0] = 1; // Master is always busy
    int i;
    for(i=1; i<poolSize; i++)   workersActivity[i] = 0;

    // pendingJobs: utilizado para contabilidad el número de jobs ya asignados pendientes de respuesta por los workers.
    int pendingJobs = howmany;

    while(howmany > 0)
    {
        int idleWorker = findIdleWorker(workersActivity, poolSize, lastAssignedWorker);
        if(idleWorker > 0)
        {
          assignWork(workersActivity, idleWorker, 1);
          lastAssignedWorker = idleWorker;
          howmany--;
        }
        else
        {
          readResultsFromWorkers(1, workersActivity);
          pendingJobs--;
        }
    }
    while(pendingJobs > 0)
    {
        readResultsFromWorkers(0, workersActivity);
        pendingJobs--;
    }
}

/*
 *
 * Esta función realiza dos tareas: por un lado hace que el Master escuche los resultados enviados por los workers y por
 * otro lado, se envía al worker que se ha recibido la información un mensaje sobre si debe seguir esperando por
 * trabajos o si ha de finalizar su contribución al sistema.
 *
 * @param goToWork indica si el worker queda en espera de más trabajo (1) o si ya puede finalizar su ejecución (0)
 * @param workersActivity el vector con el estado de actividad de los workers
 * @return
 *
 */
void readResultsFromWorkers(int goToWork, int* workersActivity)
{
    MPI_Status status;
    int size;
    int source;

    MPI_Probe(MPI_ANY_SOURCE, RESULTS_TAG, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_CHAR, &size);
    source = status.MPI_SOURCE;
    char * results = (char *) malloc(size*sizeof(char));

    MPI_Recv(results, size, MPI_CHAR, source, RESULTS_TAG, MPI_COMM_WORLD, &status);
    source = status.MPI_SOURCE;
    MPI_Send(&goToWork, 1, MPI_INT, source, GO_TO_WORK_TAG, MPI_COMM_WORLD);

    workersActivity[source]=0;
    fprintf(stdout, "%s", results);
    free(results);
}

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

// **************************************  //
// WORKERS
// **************************************  //

void
workerProcess(int myRank, struct params parameters, int maxsites)
{
    char *append(char *lhs, const char *rhs);

    int samples;
    char *results;
    char *singleResult;

    samples = receiveWorkRequest();
    results = generateSample(parameters, maxsites);
    samples--;

    while(samples > 0)
    {
        singleResult = generateSample(parameters, maxsites);
        samples--;
        results = append(results, singleResult);
    }

    sendResultsToMasterProcess(results);
    free(results); // prevent memory leaks
}

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

int isThereMoreWork() {
    int goToWork;
    MPI_Status status;

    MPI_Recv(&goToWork, 1, MPI_INT, 0, GO_TO_WORK_TAG, MPI_COMM_WORLD, &status);

    return goToWork;
}

/*
 * Prints the number of segregation sites:
 *    \n
 *    // xxx.x xx.xx x.xxxx x.xxxx
 *    segsites: xxx
 */
char *doPrintWorkerResultHeader(int segsites, double probss, struct params pars){
    char *append(char *lhs, const char *rhs);
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
 * Logic to generate a sample.
 *
 * @param samples samples to be generated
 *
 * @return the sample generated by the worker
 */
char*
generateSample(struct params parameters, unsigned maxsites)
{
    double *gensam(char **gametes, double *probss, double *ptmrca, double *pttot, struct params pars, int* segsites);
    char *doPrintWorkerResultHeader(int segsites, double probss, struct params paramters);
    char *doPrintWorkerResultPositions(int segsites, int output_precision, double *posit, char *results);

    char *doPrintWorkerResultGametes(int segsites, int nsam, char **gametes, char *results);
    int segsites;
    double probss, tmrca, ttot;
    char *results;
    char **gametes;
    double *positions;

    if( parameters.mp.segsitesin ==  0 )
        gametes = cmatrix(parameters.cp.nsam,maxsites+1);
    else
        gametes = cmatrix(parameters.cp.nsam, parameters.mp.segsitesin+1 );


    positions = gensam(gametes, &probss, &tmrca, &ttot, parameters, &segsites);

    results = doPrintWorkerResultHeader(segsites, probss, parameters);
    if(segsites > 0)
    {
        results = doPrintWorkerResultPositions(segsites, parameters.output_precision, positions, results);
        results = doPrintWorkerResultGametes(segsites, parameters.cp.nsam, gametes, results);
    }

    return results;
}


/*
 * Prints the segregation site positions:
 *      positions: 0.xxxxx 0.xxxxx .... etc.
 */
char *doPrintWorkerResultPositions(int segsites, int output_precision, double *positions, char *results){
    char *append(char *lhs, const char *rhs);
    int i;
    char tempString[3 + output_precision]; //number+decimal point+space

    results = append(results, "positions: ");

    for(i=0; i<segsites; i++){
        sprintf(tempString, "%6.*lf ", output_precision, positions[i]);
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

/*
 * Sent Worker's results to the Master process.
 *
 * @param results results to be sent
 *
 */
void sendResultsToMasterProcess(char* results)
{
    MPI_Send(results, strlen(results)+1, MPI_CHAR, 0, RESULTS_TAG, MPI_COMM_WORLD);
}

// **************************************  //
// UTILS
// **************************************  //

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

/* Initialization of the random generator. */
unsigned short * parallelSeed(unsigned short *seedv){
  unsigned short *seed48();

  return seed48(seedv);
}

/*
 * name: doInitializeRgn
 * description: En caso de especificarse las semillas para inicializar el RGN,
 *              se llama a la función commandlineseed que se encuentra en el
 *              fichero del RNG.
 *
 * @param argc la cantidad de argumentos que se recibió por línea de comandos
 * @param argv el vector que tiene los valores de cada uno de los argumentos recibidos
 */
void
doInitializeRgn(int argc, char *argv[], int *seeds, struct params parameters)
{
  int commandlineseed(char **);
  int arg = 0;

  while(arg < argc){
    switch(argv[arg++][1]){
      case 's':
        if(argv[arg-1][2] == 'e'){
          // Tanto 'pars' como 'nseeds' son variables globales
          parameters.commandlineseedflag = 1;
          *seeds = commandlineseed(argv+arg);
        }
        break;
    }
  }
}
