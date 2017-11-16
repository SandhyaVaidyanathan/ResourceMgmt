typedef struct osClock {
    unsigned long sec;
    unsigned long nsec;
}shmClock;

typedef struct pcb {
  pid_t pcbId;
  unsigned long parrivalsec; //time when process is forked
  unsigned long parrivalnsec;
  int request;
  int release;
  int deadlock;
  int terminate;
  int resources[20]; //no of resources
} shmPcb;

typedef struct resource {
  int quantity;
  int quantityAvail;
} shmRes;

typedef struct msgholder {
  long mType;
  char mText[100];
} msgholder;
