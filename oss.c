#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <getopt.h>
#include <stdbool.h>
#include "shm.h"


void spawnSlaveProcess(int);
void interruptHandler(int);
void clearSharedMem1();
void clearSharedMem2();
void clearResMem();
void clearMsg();
void resourceStats();
void cleanProcess(int);
void releaseResource (int , int );
void requestResource(int , int); 
bool req_lt_avail(int*, int);
int calcDeadlock(int , int , bool* );
int isDeadlocked(void);
void requesting();

const unsigned long int NANOSECOND = 1000000000;
const int TOTALPROCESS = 100; //default
	FILE *fp;
unsigned long startTime = 0;
int termdeadlock = 0;

pid_t childpid = 0;
int spawnedSlaves = 0;
pid_t mypid = 0;
int noOfSlaves =18;
int maxslaves = 100;
int processes = 0;
int pcbindex = -1;
char* arg1;
int shmid,shmpcbid, resid, msgid;
shmClock *shinfo;
shmPcb *shpcbinfo;
shmRes *shresinfo;
msgholder *msgqinfo;

int verbose = 0;
int ztime = 2;

int main(int argc, char const *argv[])
{

	int status = 0;
	char* logfile;
	logfile = "log.txt";
	key_t clock_key, pcb_key,res_key,msg_key;
	arg1 = (char*)malloc(40);
	srand(time(NULL));
//signal handling 
	signal(SIGINT, interruptHandler); 
	signal(SIGALRM, interruptHandler);
	signal(SIGQUIT, interruptHandler);
	alarm(2);	// 2 real seconds

	clock_key = 555;
	pcb_key = 666;    
	res_key = 777;
	msg_key =888; 
int option;

while ((option = getopt(argc, argv,"hv")) != -1) 
  	{		
  	switch (option) 
  	{		
  		case 'h' :		
           printf("Usage executable -v ie. ./oss -v would write logs to the file log.txt \n");		
           return 1;
           break;	
		case 'v':
			verbose = 1;
			break;
		default:
			printf("Please check help option for details\n");
		}
	}
    
	  //File open 
	fp = fopen(logfile, "w");
	fprintf(fp, "Opening file ... \n" );
	//Create shared memory segment for clock
	shmid = shmget(clock_key, 40*sizeof(shinfo), 0766 |IPC_CREAT |IPC_EXCL);
	if ((shmid == -1) && (errno != EEXIST)) /* real error */
	{
		perror("Unable to create shared memory");
		return -1;
	}
	if (shmid == -1)
	{
		printf("Shared Memory Already created");
		return -1;
	}
	else
	{
		shinfo = (shmClock*)shmat(shmid,NULL,0);
		if (shinfo == (void*)-1)
			return -1;
		// clock initially set to 0
		shinfo->nsec = 0;
		shinfo->sec = 0;
	}

	//create shared memory for pcb
	shmpcbid = shmget(pcb_key, 40*sizeof(shpcbinfo), 0766 |IPC_CREAT |IPC_EXCL);
	if ((shmpcbid == -1) && (errno != EEXIST)) /* real error */
	{
		perror("Unable to create shared memory");
		return -1;
	}
	if (shmpcbid == -1)
	{
		printf("Shared Memory Already created");
		return -1;
	}
	else
	{
		shpcbinfo = (shmPcb*)shmat(shmpcbid,NULL,0);
		if (shpcbinfo == (void*)-1)
			return -1;
		// clock initially set to 0
		shpcbinfo->pcbId = 0;
		shpcbinfo->parrivalsec = 0; // creation time 
		shpcbinfo->parrivalnsec = 0;
		shpcbinfo->request = -1;
		shpcbinfo->release = -1;
		shpcbinfo->deadlock = 0;

	}
//Create shared memory for resources
	resid = shmget(res_key, 40*sizeof(shresinfo), 0766 |IPC_CREAT |IPC_EXCL);
	if ((resid == -1) && (errno != EEXIST)) /* real error */
	{
		perror("Unable to create shared memory");
		return -1;
	}
	if (resid == -1)
	{
		printf("Shared Memory Already created");
		return -1;
	}
	else
	{
		shresinfo = (shmRes*)shmat(resid,NULL,0);
		if (shresinfo == (void*)-1)
			return -1;
		shresinfo->quantity = 0;
		shresinfo->quantityAvail = 0;
	}


if((msgid = msgget(msg_key, IPC_CREAT | 0777)) == -1) {
    perror("Master msgget for master queue");
    exit(-1);
  }

//Initialise process array
    int i;
  for (i = 0; i < noOfSlaves; i++) {
    shpcbinfo[i].pcbId = 0;
    shpcbinfo[i].deadlock = 0;
    shpcbinfo[i].terminate = 0;
    shpcbinfo[i].request = -1;
    shpcbinfo[i].release = -1;
    shpcbinfo[i].parrivalnsec = 0;
    shpcbinfo[i].parrivalsec = 0;
    int j;
    for(j = 0; j < 20; j++) {
      shpcbinfo[i].resources[j] = 0;
    }
  }

  //Assigning resources

  for(i = 0; i < 20; i++) {
    shresinfo[i].quantity = 1 + rand() % 10;
    shresinfo[i].quantityAvail = shresinfo[i].quantity;
  }


//Selecting 4 resources that will be shared (20% of 20 = 4)
  for(i = 0; i < 4; i++) {
    int c = rand() % 20;
    shresinfo[c].quantity = 5000;  //Assigning huge values to it
    shresinfo[c].quantityAvail = 5000;
  }

  resourceStats();


// Call child process at random intervals
	while(shinfo->sec < 20)
{
int xx = rand()%1000;
long unsigned currentTime = (shinfo->sec*NANOSECOND)+shinfo->nsec;
long unsigned plus1xxTime = currentTime + NANOSECOND +xx;
int flag = 0;

startTime = time(NULL);


    while ( currentTime <plus1xxTime)
       	{
	   		shinfo->nsec += xx;
        	if (shinfo->nsec > (NANOSECOND - 1))
            {
            shinfo->nsec -= (NANOSECOND - 1);
            	shinfo->sec++;
            }
           currentTime = (shinfo->sec*NANOSECOND)+shinfo->nsec; 
       	}
     
    //spawn process 
		 spawnSlaveProcess(1);

			xx = rand()%1000;
			plus1xxTime = currentTime + NANOSECOND +xx;

//-------------------
while (isDeadlocked())
	{
	int x,y,z,max = 0;
  for(x = 0; x < 18; x++) {
    if(shpcbinfo[x].deadlock) {
      int total = 0;
      for(y = 0; y < 20; y++) 
        total += shpcbinfo[x].resources[y];
      
      if(total > max) {
        max = total;
        z = i;
      }
    }
  }
  printf("Killing process %d\n", z);
  shpcbinfo[z].deadlock = 0;
  shpcbinfo[z].terminate = 1;
  cleanProcess(z);
  termdeadlock++;
}
shinfo->nsec +=9000;


void repeat()
{
int P, R,Q;
  if(msgrcv(msgid, (void *) &msgqinfo, sizeof(msgqinfo->mText), 3, IPC_NOWAIT) == -1) 
  	P = (errno != ENOMSG)? -1 : atoi(msgqinfo->mText); // process number stored in P

  if(P == -1) 
    return;
	R = shpcbinfo[P].request;
	if(R >= 0 )
		requestResource(R,P);
	Q = shpcbinfo[P].release; 
	if(Q >= 0 )
		releaseResource(Q,P); 
	if (shpcbinfo[P].pcbId == -1)
		cleanProcess(P);
	repeat();
}

//checking requests for all processes
requesting();

}
fclose(fp);

//clearing memory
	free(arg1);
	//kill(-getpgrp(),SIGQUIT);
	wait(NULL);
	clearSharedMem1();
	clearSharedMem2();
	clearResMem();
	clearMsg();



	return 0;
}
void resourceStats(){

	  if (verbose)
    {
      int k = 0;
      printf("5000 or more resources are alloted to shareable resources\n");
      printf("Resource# :         ");
      while(k < 20)
      {
          printf(" %d ", k );
          k++;
      }
      printf("\n");
      k = 0;
      printf("Quantity available: ");
      while(k < 20)
      {
          printf("%d ", shresinfo[k].quantityAvail);
          k++;
      }
      printf("\n");
      k = 0;
      printf("Total Quantity    : ");
      while(k < 20)
      {
          printf(" %d ", shresinfo[k].quantity );
          k++;
      }
      printf("\n");

    }
  }

void interruptHandler(int SIG){
  signal(SIGQUIT, SIG_IGN);
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT,SIG_IGN);

  if(SIG == SIGINT)
   {
    fprintf(stderr, "\nCTRL-C encoutered, killing processes\n");
  	}

  if(SIG == SIGALRM) 
  {
    fprintf(stderr, "Master has timed out. killing processes\n");
  }

  	//kill(-getpgrp(), 9);
  	clearSharedMem1();
  	clearSharedMem2();
  	clearResMem();
  	clearMsg();

  }
  void clearSharedMem1()
{
	int error = 0;
	if(shmdt(shinfo) == -1) {
		error = errno;
	}
	if((shmctl(shmid, IPC_RMID, NULL) == -1) && !error) {
		error = errno;
	}
	if(!error) {
		return ;
	}
}

void clearSharedMem2()
{
	int error = 0;
	if(shmdt(shpcbinfo) == -1) {
		error = errno;
	}
	if((shmctl(shmpcbid, IPC_RMID, NULL) == -1) && !error) {
		error = errno;
	}
	if(!error) {
		return ;
	}
}
void clearResMem()
{
		int error = 0;
	if(shmdt(shresinfo) == -1) {
		error = errno;
	}
	if((shmctl(resid, IPC_RMID, NULL) == -1) && !error) {
		error = errno;
	}
	if(!error) {
		return ;
	}
}

void clearMsg()
{
  msgctl(msgid, IPC_RMID, NULL);
}
void spawnSlaveProcess(int noOfSlaves)
{
	int i;
	mypid++;
  pcbindex = -1;
    //checking the number of processes in the pcb
    for(i = 0; i < 18; i++)
     {
      if(shpcbinfo[i].pcbId == 0)
       {
        pcbindex = i;
        shpcbinfo[i].pcbId = 1;
        break;
      } 
    }

   if(pcbindex == -1)
    	printf("Cannot spawn process, PCB is full\n");
    else
    	printf("Spawning process\n");
    	
    if(verbose)
    {
    	if(pcbindex == -1)
    		fprintf(fp, "PCB is full, cannot spawn process\n");
    	else
    		fprintf(fp, "Spawning process \n");
    }  
	//Forking processes
if(pcbindex!= -1){
	for(i = 0; i < noOfSlaves; i++) 
	{ 
    if((childpid = fork())<=0)
       		break;
       }

    if (childpid == 0)
	    {
    	//execl user.c    	
	    	shpcbinfo[pcbindex].pcbId = getpid();
			sprintf(arg1, "%d", mypid);
			execl("user", arg1, NULL); 

    	}
    	spawnedSlaves++;
     }

}

//Deadlock detection from slides
//REquest less than available check
bool req_lt_avail(int *work, int l) {
  if(shpcbinfo[l].request == -1) {
    return true;
  } 
  if(work[shpcbinfo[l].request] > 0) {
    return true;
  }
  else {
    return false;
  }
}


int calcDeadlock(int n, int m, bool* finish)
{
	printf("Calculating deadlocks\n");
int count = 0;
int p = 0;
  for(; p < n; p++) {
    if(!finish[p]) {
      shpcbinfo[p].deadlock = 1;
      count++; 
    }
    else {
      shpcbinfo[p].deadlock = 0;
    }
  }
return count;
}

int isDeadlocked(void) {
  printf("Checking for deadlocks at %llu.%llu\n", shinfo->sec,shinfo->nsec);
  const int n = 18; //no. of proccesses 
  const int m = 20; //no. of resources
  int work[m];
  bool finish[n];

  int p =0;
  for(; p < m; p++)
  {
  	    work[p] = shresinfo[p].quantityAvail;
  }


  for(p = 0; p < n; p++)
    finish[p] = false; 
  
  //For each process
  for(p = 0; p < n; p++) 
  {
  if(!shpcbinfo[p].pcbId) 
      finish[p] = true;
    
    if(finish[p]) continue;
    if(req_lt_avail(work, p))
     {
      finish[p] = true;
      int i;
      for(i = 0; i < m; i++) 
        work[i] += shpcbinfo[p].resources[i];      
      p = -1;
    }       
  }

int count = calcDeadlock(n,m,finish);
  if( count > 0) {
    printf("%d processes: ", count);
    for(p = 0; p < n; p++) 
      if(!finish[p]) 
        printf("%d ", p);

    printf("are deadlocked\n");
    return count;
  }
  else {
    printf("No deadlocks found :)\n");
    return count;
  }
}


void cleanProcess(int i) {
//Getting resources from a dead process
  int j =0;
  while( j < 20) {
    if(shpcbinfo[i].resources[j] > 0) {
      //Get the number of resource to return
      int resourcereturn = shpcbinfo[i].resources[j];
      //Add it to available resource
      shresinfo[j].quantityAvail += resourcereturn;
      printf(" %d Resources(R %d) are released from process %d.\n", resourcereturn, j, i);
      if(verbose) 
      {
      //  printf(" %d Resources(R %d) are released from process %d.\n", resourcereturn, j, i);
        fprintf(fp," %d Resources(R %d) are released from process %d.\n", resourcereturn, j, i);
        fprintf(fp,"There are now %d out of %d for R%d . \n", shresinfo[j].quantityAvail, shresinfo[j].quantity, j);
		}
      //Reinitialize all resources to 0
      shpcbinfo[i].resources[j] = 0;
    } 
    j++;
  }
  shpcbinfo[i].pcbId = 0;
  shpcbinfo[i].request = -1;
  shpcbinfo[i].release = -1;
  shpcbinfo[i].parrivalnsec = 0;
  shpcbinfo[i].parrivalsec = 0;
  shpcbinfo[i].terminate = 0;
}

void requestResource(int j, int i) {
  int avail_res = shresinfo[j].quantityAvail;
  if(avail_res  > 0)
   {//grant request and reduce availability
    shpcbinfo[i].resources[j]++;
    shpcbinfo[i].request = -1;
    shresinfo[j].quantityAvail--;
    printf("Process %d is granted Resource # %d  at time %lu.%lu \n", i , j, shinfo->sec, shinfo->nsec);
  }
  else
  	printf("Resource %d is not available at the moment \n",j );
}

void releaseResource (int j, int i)
{
	if(shpcbinfo[i].resources[j] >0)
   {
  shpcbinfo[i].resources[j]--;
  shresinfo[j].quantityAvail++;
  shpcbinfo[i].release = -1;
  printf("Released resource %d from process %d at time %lu.%lu .\n", j , i, shinfo->sec, shinfo->nsec );
	}
	else
		printf("No resource to release \n");
}

void requesting(void) {

  int i;
  for(i = 0; i < 18; i++) {
    int resource = -1;
    if((resource = shpcbinfo[i].request) >= 0) 
      requestResource(resource, i);
    
    else if((resource = shpcbinfo[i].release) >= 0) 
      releaseResource(resource, i);
    
    //If the process set its pcbId is to -1 collect reources it had back to the pool
    else if(shpcbinfo[i].pcbId == -1){
      cleanProcess(i);    
    }

  }

}