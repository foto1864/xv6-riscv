#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

struct pstat {
  int pid[NPROC];
  int ppid[NPROC];
  int state[NPROC];
  int priority[NPROC];
  uint64 sz[NPROC];
  char name[NPROC][16];
  int used[NPROC];
};

static char*
stname(int s)
{
  switch(s){
  case 0: return "UNUSED";
  case 1: return "USED";
  case 2: return "SLEEPING";
  case 3: return "RUNNABLE";
  case 4: return "RUNNING";
  case 5: return "ZOMBIE";
  default: return "?";
  }
}

int
main(int argc, char *argv[])
{
  struct pstat ps;

  if(getpinfo(&ps) < 0){
    fprintf(2, "ps: getpinfo failed\n");
    exit(1);
  }

  printf("PID\tPPID\tPRIO\tSTATE\tSZ\tNAME\n");
  for(int i = 0; i < NPROC; i++){
    if(ps.used[i]){
      printf("%d\t%d\t%d\t%s\t%ld\t%s\n",
             ps.pid[i],
             ps.ppid[i],
             ps.priority[i],
             stname(ps.state[i]),
             (long)ps.sz[i],
             ps.name[i]);
    }
  }

  exit(0);
}
