#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include "datastructs.h"

#ifdef DBG_VTE
#define D(x) x;         //Include given instruction in the sources
#define NO_W(x)
#else
#define D(x)
#define NO_W(x) (void)x;    //Suppress -Wunused-variable warnings
#endif

#define NO_TIMER -1

typedef enum {
  RR_CRITERIA,
  SJN_CRITERIA
} sched_policy;

void scheduler(char label, void *shmem, u_int num_proc, char *outfile, u_char quantum, sched_policy sp, char core_n);

#endif /*__SCHEDULER_H__*/
