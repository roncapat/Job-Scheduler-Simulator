# Job scheduler simulator
A simplified job scheduler simulator written in C as part of a University assignment.

# Requirements

## Introduction
Create a program (from now on "simulator") in C that simulates two schedulers.
One with preemption, the other without.
The objective of the simulation is to provide a comparison between the two approaches.
The program must simulate the entities education, job, processor, core, clock and scheduler.

## Functionalities
When starting the simulator, you need to create 3 processes:
* master
* scheduler_preemptive
* scheduler_not_preemptive


The "master" process reads the jobs to be executed from file and forwards them to both schedulers.
The scheduler with preemption adopts the Round Robin strategy, the duration of the quantum (time
slice) will be provided as a parameter to the simulator, while the scheduler without preemption
adopts the Shortest Job Next (SJN) strategy.
"Shortest Job" means the Job with the lowest sum of the length of instructions they are
yet to be executed.
A scheduler ends when there are no more jobs to execute.
Regardless of the strategy, when an instruction is suspended due to an operation
blocker, another job must be scheduled.

## Entities
The processor of each scheduler has two cores. Therefore we will have to use 2 (real) threads for
simulate the execution of 2 jobs in parallel. Each core will then have its own clock simulated with
an integer that increases with each cycle. 

An instruction consists of:
* type_flag:
  * Flag 0: = calculation instruction, or non-blocking;
  * Flag 1: = I/O operation, or blocking;
* length: the duration of an instruction execution in clock cycles;
* io_max: if an instruction is blocking, the job is blocked for a random number between 1 and io_max clock cycles.

A job is defined by:
* id: unique id;
* arrival_time: integer describing at which clock cycle the job must be considered by the scheduler;
* instr_list: the list of instructions to be executed;
* state: the state (new, ready, running, blocked, exit).

All jobs read from files will have a "new" status.

A scheduler MUST NOT run a job if: clock(core) < clock(job)

A job ends when it no longer has instructions to execute.


## Command line parameters
The simulator accepts the following parameters (mandatory) from the command line:
1. -op | --output-preemption: the output file with the scheduler results with preemption;
2. -on | --output-no-preemption: the output file with the scheduler results without preemption;
3. -i | --input: the input file containing the list of jobs. At the end of the file scan, jobs must be sent to the two schedulers in the same order in which they were read;
4. -q | --quantum: the duration of a quantum of time (measured in clock cycles) on the scheduler with preemption.

For example:

`./simulator -op out_pre.csv -on out_not_pre.csv -i 6_jobs.dat -q 6`

If these parameters are not supplied, the simulator terminates showing a message that describes the use of the simulator.


## Inputs
The list of jobs (sorted by "arrival_time") and related instructions is present in the "input jobs" subfolder of this repository.

Each line begins with "j" or "i"
* j -> job
  * j, id, arrival_time
  * followed by its "i" instructions
* i -> education
  * i, type_flag, length, io_max
  
For example, a file with the following content:

```
j, 0.2
i, 1,6,6
i, 1,3,8
j, 1.23
i, 0,4,0
```

contains 2 jobs, the first formed by two blocking instructions, the second by a blocking instruction.
The files are named: `x_jobs.dat` with x = 01, 02, ..., 10.
The tests will be performed using the file number as long as, for example, the file `07_jobs.dat` will be tested with `-q 7`.


## Output
Each time the status of a job changes, each scheduler must log the event on the file
corresponding and the output must be STRICTLY formatted as described:

`core, clock, job_id, status`

Examples:
```
core0,123,456, running
core1,42,33, exit
```
