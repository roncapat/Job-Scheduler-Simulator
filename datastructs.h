#ifndef __DATASTRUCTS_H__
#define __DATASTRUCTS_H__

#define NO_CORE_WAITING -1

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

typedef enum {
  PROC_NEW,
  PROC_READY,
  PROC_RUNNING,
  PROC_BLOCKED,
  PROC_EXIT
} exec_status;

typedef enum {
  INSTR_CALC,
  INSTR_IO
} instr_type;

typedef struct instruction {
  instr_type type;
  u_char length;
  u_char io_max;
  struct instruction *next_instruction;
} instruction;

typedef struct process {
  u_short id;
  u_int arrival_time;
  exec_status status;
  char waiting_core_id;
  u_int instr_count;
  u_int total_cycles;
  instruction *instruction_ptr;
  struct process *next_process;
} process;

typedef struct queue {
  char core_n;
  process *first;
  process *last;
  u_int *earliest;
} queue;

const u_short size_of_proc;
const u_short size_of_inst;

#endif /*__DATASTRUCTS_H__*/
