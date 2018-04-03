#ifndef __SCHEDULER_HELPERS_H__
#define __SCHEDULER_HELPERS_H__

#include "datastructs.h"
#include "core.h"
#include "scheduler.h"

void load_job            (FILE *logfile, FILE *output, core *c, queue *ready);
void unload_job          (FILE *logfile, FILE *output, core *c, queue *ready, queue * waiting, sched_policy sp, unsigned int *ended);
void flush_waiting_queue (FILE *logfile, FILE *output, core *c, queue *ready, queue * waiting, sched_policy sp);
void forward_clock       (               FILE *output, core *c, queue *q);

#endif /*__SCHEDULER_HELPERS_H__*/
