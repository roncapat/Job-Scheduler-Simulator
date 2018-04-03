#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sysexits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#include "scheduler.h"
#include "scheduler_helpers.h"
#include "datastructs.h"
#include "datastructs_helpers.h"
#include "core.h"


void scheduler (char label, void* shmem, u_int num_proc, char* outfile, u_char quantum, sched_policy sp, char core_n){

    printf ("[%c] Initializing structures and streams...\n", label);

    // DEBUG PIPE AND DISPLAY SETUP
    FILE *dbgpipe = NULL;
    #ifdef DBG_VTE
        char pipename[14];
        sprintf (pipename, "/tmp/sched_%d", getpid ()%100);
        mkfifo (pipename, 0666);

        dbgpipe = fopen (pipename, "w+");
        setvbuf (dbgpipe, NULL, _IONBF, 0);

        if (fork () == 0) {
            char* argv[] = {"gnome-terminal", "-x", "cat", pipename, '\0'};
            execvp ("gnome-terminal", argv);
        }
    #endif

    // LOGFILE SETUP
    FILE *logfile = fopen (outfile, "w+");
    printf ("[%c] Logfile: %s\n", label, outfile);
    if (logfile == NULL) {
        perror ("Failed to create or truncate log file");
        exit (EX_OSFILE);
    }

    // QUEUES SETUP
    queue ready, waiting;
    queue_init (&ready, core_n);
    queue_init (&waiting, core_n);

    waiting.first = (process*) shmem;
    process *p = waiting.first;
    for (u_int i = 1; i < num_proc; i++)
        p = p->next_process;
    waiting.last = p;

    for (int i = 0; i<core_n; i++){
        queue_update_earliest_job (&waiting, i);
    }

    //PROCESSOR SETUP
    core cores[core_n];

    for (int i = 0; i<core_n; i++){
        core_init (&cores[i], i, quantum);
    }

    u_int jobs_ended = 0;

    // SCHEDULER LOOP
    printf ("[%c] Entering runtime loop...\n", label);
    while (jobs_ended < num_proc){
        for (int i = 0; i<core_n; i++){
            core *c = &cores[i];
            int has_ended = 0;
            char nothing_to_do = 0;
            sem_getvalue (&(c->end), &has_ended);
            if (has_ended){

                // \033[2J\033[H is ANSII escape to clean screen
                D( fprintf (dbgpipe, "\033[2J\033[H############## SCHED-%c TARGETING CORE #%d ##############\n", label, c->id) )
                sem_wait (& (c->end));
                D( fprintf (dbgpipe, "PROGRESS:     %u/%u\n", jobs_ended, num_proc) )
                D( fprintf (dbgpipe, "CLK:          %u\n", c->clk) )

                // !!! see schema n.3 to better understand scheduler logic
                if (!queue_no_job_available (&waiting, c) && c->clk >= waiting.earliest[c->id]){
                    flush_waiting_queue (logfile, dbgpipe, c, &ready, &waiting, sp);
                    unload_job (logfile, dbgpipe, c, &ready, &waiting, sp, &jobs_ended);
                } else {
                    unload_job (logfile, dbgpipe, c, &ready, &waiting, sp, &jobs_ended);
                    if (queue_no_job_available (&ready, c)){
                        if (queue_no_job_available (&waiting, c)){
                            nothing_to_do = 1;
                        } else {
                            forward_clock (dbgpipe, c, &waiting);
                            flush_waiting_queue (logfile, dbgpipe, c, &ready, &waiting, sp);
                        }
                    } else {
                        if (c->clk < ready.earliest[c->id]){
                            if (queue_no_job_available (&waiting, c)){
                                forward_clock (dbgpipe, c, &ready);
                            } else {
                                if (waiting.earliest[c->id] <= ready.earliest[c->id]){
                                    forward_clock (dbgpipe, c, &waiting);
                                    flush_waiting_queue (logfile, dbgpipe, c, &ready, &waiting, sp);
                                } else {
                                    forward_clock (dbgpipe, c, &ready);
                                }
                            }
                        } else {
                        }
                    }
                }

                D( fprintf (dbgpipe,"CLK:          %u\n", c->clk) )
                if (!nothing_to_do)
                    load_job (logfile, dbgpipe, c, &ready);

                sem_post (& (c->start));
                D( if (!nothing_to_do) 
						fprintf (dbgpipe,"START\n"); 
                   else 
						fprintf (dbgpipe,"SLEEP\n"); )
            }
        }
    }
	
	for (int i = 0; i<core_n; i++) {
		int ret = core_stop(&cores[i]);
		if (ret != 0){
			char str[40];
			errno = ret; 
			sprintf(str, "[%c] Failed to join a core_runtime", label);
			perror(str); 
			exit(EXIT_FAILURE);
		}    
	}

    assert (jobs_ended == num_proc);
    printf ("[%c] All jobs exited\n", label);

    printf ("[%c] Cleanup...\n", label);

    queue_destroy(&ready);
    queue_destroy(&waiting);
    fclose (logfile);

    #ifdef DBG_VTE
        fclose (dbgpipe);
        unlink (pipename);
    #endif

    printf ("[%c] Done. Goodbye!\n", label);
}
