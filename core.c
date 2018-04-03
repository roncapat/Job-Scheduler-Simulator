#include <stdlib.h>

#include "core.h"
#include <assert.h>
#include <stdio.h>

static void* __core_runtime (void* core_ptr){
    core * const c = (core *) core_ptr;
    instruction * i = NULL;
    int remaining_time = -1;

    while (1) {
        sem_wait (& (c->start));

        if (c->should_exit) {
            return ((void*)0);
        }

        if (c->cur_proc != NULL){
            remaining_time = c->timeout;

            while (1){
                i = c->cur_proc->instruction_ptr;

                if (i == NULL) {
                    c->cur_proc->status = PROC_EXIT;
                    break;
                }

                if (remaining_time > 0  && remaining_time < i->length) { //only preempt
                    c->clk += remaining_time;
                    c->cur_proc->total_cycles -= remaining_time;
                    i->length -= remaining_time;
                    remaining_time = 0;
                } else if (remaining_time == i->length){                //only preempt
                    c->clk += remaining_time;
                    c->cur_proc->total_cycles -= remaining_time;
                    c->cur_proc->instruction_ptr = i->next_instruction;
                    i->length = 0;
                    remaining_time = 0;
                } else {                                                //both preeempt & not preeempt
                    c->clk += i->length;
                    c->cur_proc->total_cycles -= i->length;
                    c->cur_proc->instruction_ptr = i->next_instruction;
                    if (remaining_time != -1) {                         // only if preempt
                        remaining_time -= i->length;
                    }
                    i->length = 0;
                }

                if (c->cur_proc->instruction_ptr == NULL){
                    c->cur_proc->status = PROC_EXIT;
                    break;
                } else if (i->type == INSTR_IO && i->length == 0) {
                    c->cur_proc->arrival_time    = c->clk + ( rand () % i->io_max ) + 1;
                    c->cur_proc->status          = PROC_BLOCKED;
                    c->cur_proc->waiting_core_id = c->id;
                    break;
                } else if (remaining_time == 0) {
                    c->cur_proc->status = PROC_READY;
                    c->cur_proc->waiting_core_id = NO_CORE_WAITING;
                    break;
                }
            }
        }

        sem_post (& (c->end));
    }
    assert(0); // Must never exit runtime loop without notice
    return ((void*)1);
}

inline int core_init (core *c, short id, char timeout) {
    sem_init (& (c->start), 0, 1);
    sem_init (& (c->end),   0, 1);
    sem_wait (& (c->start));
    c->id = id;
    c->timeout = timeout;
    c->clk = 0;
    c->cur_proc    = NULL;
    c->should_exit = 0;

    return pthread_create (& (c->trd), NULL, __core_runtime, (void *)c);
}

inline int core_stop (core *c){
	sem_wait (& (c->end));
	c->should_exit = 1;
	sem_post (& (c->start));
	return pthread_join (c->trd, NULL);
}
