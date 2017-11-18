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

void interruptHandler(int);
void clearSharedMem1();
void clearSharedMem2();
void clearResMem();
void clearMsg();

const unsigned long int NANOSECOND = 1000000000;

int shmid,shmpcbid, resid, msgid;
shmClock *shinfo;
shmPcb *shpcbinfo;
shmRes *shresinfo;
msgholder *msgqinfo;
unsigned long startTime = 0;

int main(int argc, char const *argv[])
{

	key_t clock_key, pcb_key,res_key,msg_key;
	clock_key = 555;
	pcb_key = 666;    
	res_key = 777;
	msg_key =888; 

	signal(SIGINT, interruptHandler); 
	signal(SIGALRM, interruptHandler);

return 0;
}