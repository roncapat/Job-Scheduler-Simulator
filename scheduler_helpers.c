#include <stdio.h>
#include <assert.h>

#include "scheduler.h"
#include "scheduler_helpers.h"
#include "datastructs.h"
#include "datastructs_helpers.h"
#include "core.h"

void load_job(FILE *logfile, FILE *dbgpipe, core *c, queue *ready) {
  NO_W(dbgpipe)
  assert(queue_no_job_available(ready, c) == 0);
  c->cur_proc = queue_remove_first_elegible(ready, c);
  assert(c->cur_proc != NULL);
  c->cur_proc->status = PROC_RUNNING;
  fprintf(logfile, "core%hu,%u,%hu,running\n", c->id, c->clk, c->cur_proc->id);
  D(fprintf(dbgpipe, "LOAD:         Job #%hu selected and loaded\n", c->cur_proc->id))
}

void unload_job(FILE *logfile, FILE *dbgpipe, core *c, queue *ready, queue *waiting, sched_policy sp, u_int *exited) {
  NO_W(dbgpipe)
  if (c->cur_proc != NULL) { //NULL only when core is started first time
    assert(c->cur_proc->status != PROC_NEW
               && c->cur_proc->status != PROC_RUNNING);
    switch (c->cur_proc->status) {
      case PROC_READY:
        switch (sp) {
          case RR_CRITERIA:queue_insert_tail(ready, c->cur_proc);
            break;
          case SJN_CRITERIA:queue_insert_sjn(ready, c->cur_proc);
            break;
        }
        fprintf(logfile, "core%hu,%u,%hu,ready\n", c->id, c->clk, c->cur_proc->id);
        D(fprintf(dbgpipe, "UNLOAD:       Job #%hu re-enqueued\n", c->cur_proc->id))
        break;
      case PROC_BLOCKED:queue_insert_latn(waiting, c->cur_proc);
        fprintf(logfile, "core%hu,%u,%hu,blocked\n", c->id, c->clk, c->cur_proc->id);
        D(fprintf(dbgpipe, "UNLOAD:       Job #%hu blocked until %u\n", c->cur_proc->id, c->cur_proc->arrival_time))
        break;
      case PROC_EXIT:(*exited)++;
        fprintf(logfile, "core%hu,%u,%hu,exit\n", c->id, c->clk, c->cur_proc->id);
        D(fprintf(dbgpipe, "UNLOAD:       Job #%hu exited\n", c->cur_proc->id))
        break;
      default: //Should never jump here, but needed to suppress switch enum warning
        break;
    }
    c->cur_proc = NULL;
  }
}

void flush_waiting_queue(FILE *logfile, FILE *dbgpipe, core *c, queue *ready, queue *waiting, sched_policy sp) {
  NO_W(dbgpipe)
  int flushed = 0;
  process *p = NULL;
  while ((p = queue_remove_first_elegible(waiting, c)) != NULL) {
    switch (sp) {
      case RR_CRITERIA:queue_insert_tail(ready, p);
        break;
      case SJN_CRITERIA:queue_insert_sjn(ready, p);
        break;
    }
    p->status = PROC_READY;
    fprintf(logfile, "core%hu,%u,%hu,ready\n", c->id, p->arrival_time, p->id);
    D(fprintf(dbgpipe, "FLUSH:        Job #%hu moved to ready queue\n", p->id))
    flushed++;
    if (queue_no_job_available(waiting, c)) {
      D(fprintf(dbgpipe, "FLUSH:        No more jobs waiting\n"))
      break;
    }
  }
  assert(flushed);
}

void forward_clock(FILE *dbgpipe, core *c, queue *q) {
  NO_W(dbgpipe)
  c->clk = q->earliest[c->id];
  D(fprintf(dbgpipe, "FWD:          Clock jumped to the first available job (%u)\n", c->clk))
}
