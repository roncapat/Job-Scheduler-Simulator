#ifndef __CORE_H__
#define __CORE_H__

#include <pthread.h>
#include <semaphore.h>

#include "datastructs.h"

typedef struct core {
  pthread_t trd;
  sem_t start;
  sem_t end;
  u_short id;
  u_int clk;
  process *cur_proc;
  char timeout;
  char should_exit;
} core;

int core_init(core *c, short id, char timeout);
int core_stop(core *c);

#endif /*__CORE_H__*/
