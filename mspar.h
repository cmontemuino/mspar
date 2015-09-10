struct message {
	char *resultHeader;
};

void masterProcessingLogic(int howmany, int lastIdleWorker, int poolSize);
char* workerProcessingLogic(int myRank, int samples, struct params pars, unsigned maxsites);
void doInitializeRgn(int argc, char *argv[]);
void sendResultsToMasterProcess(int myRank, char* results);
int receiveWorkRequest();
void doInitGlobalDataStructures(int argc, char *argv[], int *howmany);
void assignWork(int* workersActivity, int assignee, int samples);
void readResultsFromWorkers(int goToWork, int* workersActivity);
int findIdleWorker(int* workersActivity, int poolSize, int lastAssignedWorker);

/* From ms.c*/
char ** cmatrix(int nsam, int len);

/*
void ordran(int n, double pbuf[]);
void ranvec(int n, double pbuf[]);
void order(int n, double pbuf[]);

void biggerlist(int nsam,  char **list );
int poisso(double u);
void locate(int n,double beg, double len,double *ptr);
void mnmial(int n, int nclass, double p[], int rv[]);
void usage();
int tdesn(struct node *ptree, int tip, int node );
int pick2(int n, int *i, int *j);
int xover(int nsam,int ic, int is);
int links(int c);
*/