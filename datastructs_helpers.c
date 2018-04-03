#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "datastructs.h"
#include "datastructs_helpers.h"
#include "core.h"


const u_short size_of_proc = sizeof (process);
const u_short size_of_inst = sizeof (instruction);


inline void process_init (process *p) {
    p->id              = 0;
    p->arrival_time    = 0;
    p->status          = PROC_NEW;
    p->waiting_core_id = NO_CORE_WAITING;
    p->instr_count     = 0;
    p->total_cycles    = 0;
    p->instruction_ptr = NULL;
    p->next_process    = NULL;
}


inline void instruction_init (instruction *i) {
    i->type             = INSTR_CALC;
    i->length           = 0;
    i->io_max           = 0;
    i->next_instruction = NULL;
}


inline void queue_init (queue *q, char core_n) {
    q->core_n = core_n;
    q->first  = NULL;
    q->last   = NULL;
    q->earliest = malloc (sizeof(u_int) * core_n);
    for (int i = 0; i<core_n; i++){
        q->earliest[i] = 0;
    }
}

inline void queue_destroy(queue *q){
    free(q->earliest);
}

static inline int __queue_is_coherent (queue *q) {
    return (    (q->first == NULL && q->last == NULL)
             || (q->first != NULL && q->last != NULL) );
}



inline char queue_is_empty (queue *q) {
    if (q->first == NULL && q->last == NULL)  return 1;
    else return 0;
}

inline char queue_no_job_available (queue *q, core *c) {
    if (q->earliest[c->id] == 0) return 1;
    else return 0;
}

static inline void __queue_set_earliest_job_after_insertion (queue *q, process *p){
    assert (__queue_is_coherent (q));
    assert(!queue_is_empty(q));

    for (int i = 0; i< q->core_n; i++){
        if (p->waiting_core_id == NO_CORE_WAITING || p->waiting_core_id == i){
            if (q->earliest[i] == 0 || p->arrival_time < q->earliest[i]) {
                q->earliest[i] = p->arrival_time;
            }
        }
    }

    assert (__queue_is_coherent (q));
}


static inline void __queue_set_earliest_job_after_removal (queue *q, process *p){
    assert (__queue_is_coherent (q));

    for (int i = 0; i< q->core_n; i++){
		/// When removing a job that is not the earliest, the earliest is the same.
		/// Need to search only if the earliest job is removed.
        if (p->arrival_time == q->earliest[i]){
			if (p->waiting_core_id == NO_CORE_WAITING || p->waiting_core_id == i){
				queue_update_earliest_job(q, i);
			}
		}
    }

    assert (__queue_is_coherent (q));
}

//Do a linear search of the earliest job available for each core
inline void queue_update_earliest_job (queue *q, u_short core_id) {
    assert (__queue_is_coherent (q));

	q->earliest[core_id] = 0;
	if (!queue_is_empty (q)) {
		char first = 0;
		for (process *p = q->first; p != NULL; p = p->next_process){
			if (p->waiting_core_id == NO_CORE_WAITING || p->waiting_core_id == core_id){
				if (p->arrival_time < q->earliest[core_id] || first == 0) {
					q->earliest[core_id] = p->arrival_time;
					first = 1;
				}
			}
		}
	}
	
    assert (__queue_is_coherent (q));
}



// Add the specified process to the end of the queue.
inline void queue_insert_tail (queue* q, process* p) {
    assert (__queue_is_coherent (q));

    if (queue_is_empty (q)) {
        q->first = p;
        q->last = p;
        p->next_process = NULL;
    } else {
        q->last->next_process = p;
        q->last = p;
    }

    __queue_set_earliest_job_after_insertion (q, p);

    assert (__queue_is_coherent (q));
}

// Add the specified process to a queue, assuming these criteria:
// - The nodes in the queue are already ordered by minimum amount of clock cycles needed;
// - The specified process will be placed among them, without disrupting ordering policy.
inline void queue_insert_sjn (queue* q, process* p) {
    assert (__queue_is_coherent (q));

    if (queue_is_empty (q)) {
        q->first = p;
        q->last = p;
        p->next_process = NULL;
    } else {
        process *prev   = NULL,
                *cursor = q->first;

        while (cursor->total_cycles < p->total_cycles){
            prev   = cursor;
            cursor = cursor->next_process;
            if (cursor == NULL) break;
        }

        if (prev == NULL){              //Insert as first
            p->next_process = q->first;
            q->first = p;
        } else if (cursor == NULL){     //Insert as last
            q->last->next_process = p;
            q->last = p;
        } else {                        //Insert in the middle
            prev->next_process = p;
            p->next_process = cursor;
        }
    }

    __queue_set_earliest_job_after_insertion (q, p);

    assert (__queue_is_coherent (q));
}


// Add the specified process to a queue, assuming these criteria:
// - The nodes in the queue are already ordered by arrival time;
// - The specified process will be placed among them, without disrupting ordering policy.
inline void queue_insert_latn (queue* q, process* p) {
    assert (__queue_is_coherent (q));

    if (queue_is_empty (q)) {
        q->first = p;
        q->last = p;
        p->next_process = NULL;
    } else {
        process *prev   = NULL,
                *cursor = q->first;

        while (cursor->arrival_time < p->arrival_time){
            prev   = cursor;
            cursor = cursor->next_process;
            if (cursor == NULL) break;
        }

        if (prev == NULL){              //Insert as first
            p->next_process = q->first;
            q->first = p;
        } else if (cursor == NULL){     //Insert as last
            q->last->next_process = p;
            q->last = p;
        } else {                        //Insert in the middle
            prev->next_process = p;
            p->next_process = cursor;
        }
    }

    __queue_set_earliest_job_after_insertion (q, p);

    assert (__queue_is_coherent (q));
}


// Find a process in the queue, which has "arrival_time" lower than the specified clock.
// Remove and return that process node.
inline process* queue_remove_first_elegible (queue *q, core *c) {
    assert (__queue_is_coherent (q));

    if ( queue_no_job_available (q, c) ) {
        return NULL;
    }

    process *prev   = NULL,
            *cursor = q->first;
    char found = 0;

    while(cursor != NULL) {
        if(cursor->arrival_time <= c->clk){
            if (cursor->waiting_core_id == NO_CORE_WAITING || cursor->waiting_core_id == c->id){
                found = 1;
                break;
            }
        }
        prev = cursor;
        cursor = cursor->next_process;
    }

    if ( !found ) {
        return NULL;
    }

    if (cursor == q->first && cursor == q->last) {   //Only element
        q->first = q->last = NULL;
        cursor->next_process = NULL;
    } else if (cursor == q->first) {                 //First element
        q->first = cursor->next_process;
        cursor->next_process = NULL;
    } else if (cursor == q->last) {                  //Last element
        q->last = prev;
        prev->next_process = NULL;
    } else if (prev != NULL) {                       //Otherwise
        prev->next_process = cursor->next_process;
        cursor->next_process = NULL;
    }

    __queue_set_earliest_job_after_removal (q, cursor);

    assert (__queue_is_coherent (q));
    assert (cursor != NULL);
    return cursor;
}
