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
//void sendMessage(int , int );

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
	pid_t mypid = atoi(argv[0]);
	srand(time(NULL));
	//Create shared memory segment for clock
	shmid = shmget(clock_key, 40*sizeof(shinfo), 0766 |IPC_EXCL);
	if ((shmid == -1) && (errno != EEXIST)) 
	{
		perror("Unable to read shared memory");
		return -1;
	} 
	else	
	{
		shinfo = (shmClock*)shmat(shmid,NULL,0);
		if (shinfo == (void*)-1)
			return -1;
	}

	//create shared memory for pcb
	shmpcbid = shmget(pcb_key, 40*sizeof(shpcbinfo), 0766 |IPC_EXCL);
	if ((shmpcbid == -1) && (errno != EEXIST)) /* real error */
	{
		perror("Unable to read shared memory");
		return -1;
	}
	else
	{
		shpcbinfo = (shmPcb*)shmat(shmpcbid,NULL,0);
		if (shpcbinfo == (void*)-1)
			return -1;

	}
//Create shared memory for resources
	resid = shmget(res_key, 40*sizeof(shresinfo), 0766 |IPC_EXCL);
	if ((resid == -1) && (errno != EEXIST)) /* real error */
	{
		perror("Unable to read shared memory");
		return -1;
	}
	else
	{
		shresinfo = (shmRes*)shmat(resid,NULL,0);
		if (shresinfo == (void*)-1)
			return -1;
	}


if((msgid = msgget(msg_key, IPC_CREAT | 0777)) == -1) {
    perror(" msg user queue");
    exit(-1);
  }


	signal(SIGINT, interruptHandler); 
	signal(SIGALRM, interruptHandler);
	signal(SIGQUIT, interruptHandler);
	alarm(2);	// 2 real seconds

void sendMessage(int qid, int msgtype) {
  struct msgholder msg;
  msg.mType = msgtype;
  sprintf(msg.mText, "%d", mypid);

  if(msgsnd(qid, (void *) &msg, sizeof(msg.mText), IPC_NOWAIT) == -1) 
    perror("    Slave msgsnd error");
  
}
  int i = 0;
  bool finish = 0;
  do {
    //If this process request is -1, it need not wait 
    if(shpcbinfo[mypid].request == -1 && shpcbinfo[mypid].release == -1) {
     int nsecdiff  = shinfo->sec - shpcbinfo[mypid].parrivalsec ;
    int term;
  if(nsecdiff >=1) {

    term = 1 + rand() % 3;
  }  	//randomly terminating process
      if(term == 1) 
      {
      	   finish = true;
      }
      else {
        if(rand()%2) {
          int option = rand() % 2;
          //Request a resource
          if(option) {
          shpcbinfo[mypid].request = rand()%3; 
           sendMessage(msgid, 3);
          }
          //Release a resource
          else {
            int i;
            for(i = 0; i < 20; i++) {
              if(shpcbinfo[mypid].resources[i] > 0) {
                shpcbinfo[mypid].release = i;
                break;
              }
            }
            sendMessage(msgid, 3);
          }
        }
      }
    }
  } while (!finish  && !shpcbinfo[mypid].terminate);

  if(!shpcbinfo[mypid].terminate) {
    shpcbinfo[mypid].pcbId = -1;
    sendMessage(msgid, 3);
  }
 
sleep(5);
  kill(mypid, SIGTERM);
  sleep(1);
  kill(mypid, SIGKILL);


return 0;
}

void interruptHandler(int SIG){
  signal(SIGQUIT, SIG_IGN);
  signal(SIGINT, SIG_IGN);

  if(SIG == SIGINT)
   {
    fprintf(stderr, "\nCTRL-C encoutered, killing processes\n");
  	}

  if(SIG == SIGALRM) 
  {
    fprintf(stderr, "Master has timed out. killing processes\n");
  }

  	//kill(-getpgrp(), 9);
  	wait(NULL);
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
