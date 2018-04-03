#ifndef __DATASTRUCTS_HELPERS_H__
#define __DATASTRUCTS_HELPERS_H__

#include "datastructs.h"
#include "core.h"

void     process_init     (process *p);
void     instruction_init (instruction *i);
void     queue_init       (queue *q, char core_n);

void     queue_destroy(queue *q);

char     queue_is_empty         (queue *q);
char 	 queue_no_job_available (queue *q, core *c);

void     queue_update_earliest_job (queue *q, u_short core_id);

void     queue_insert_tail (queue* q, process* p);
void     queue_insert_sjn  (queue* q, process* p);
void     queue_insert_latn (queue* q, process* p);

process* queue_remove_first_elegible  (queue *p, core *c);

#endif /*__DATASTRUCTS_HELPERS_H__*/
