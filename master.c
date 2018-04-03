#include <errno.h>       
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>        
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <getopt.h>         
#include <sysexits.h>       

#include "datastructs.h"
#include "datastructs_helpers.h"
#include "scheduler.h"


void      print_help (char* arg0);
void      parse_cmdline (int argc, char **argv, char **i_filename, char **p_filename, char **n_filename, u_char *quantum);
u_long    estimate_requirements (char *i_filename, u_int *proc_num, u_int *instr_num);
void *    setup_shared_mem (u_long required_mem);
void      populate_shared_mem (void *shared_mem, char *i_filename);
pid_t     launch_scheduler (char label, void *shared_mem, u_int proc_num, char *outfile, u_char quantum, sched_policy sp, char core_n);
char      wait_for_subprocesses();


int main (int argc, char **argv){
    char           * preempt_outfile    = "",
				   * nonpreempt_outfile = "",
				   * infile             = "";
	u_char           quantum            = 0;
	u_int            proc_num           = 0,
					 instr_num          = 0;
	u_long           required_mem       = 0;
	void           * shared_mem         = NULL;
    
	parse_cmdline (argc, argv, &infile, &preempt_outfile, &nonpreempt_outfile, &quantum);
    
    required_mem = estimate_requirements (infile, &proc_num, &instr_num);
    shared_mem = setup_shared_mem (required_mem);
    populate_shared_mem (shared_mem, infile);
	

	printf ("[M] Spawning subprocesses...\n");
	launch_scheduler ('N', shared_mem, proc_num, nonpreempt_outfile, NO_TIMER, SJN_CRITERIA, 2);
    launch_scheduler ('P', shared_mem, proc_num, preempt_outfile, quantum, RR_CRITERIA, 2);
    if (wait_for_subprocesses() != 0) 
		exit(EXIT_FAILURE);
    
    ///scheduler ('N', shared_mem, proc_num, nonpreempt_outfile, NO_TIMER, SJN_CRITERIA, 2);
    ///scheduler ('P', shared_mem, proc_num, preempt_outfile, quantum, RR_CRITERIA, 2);
	
	printf ("[M] Cleanup...\n");
    munmap (shared_mem,required_mem);
    unlink ("/tmp/__RESERVED");
    
    printf ("[M] Done. Goodbye!\n");

    return (0);
}



void print_help (char* arg0){
    printf ("[M]   Usage:\n");
    printf ("[M]     %s <arg list>\n", arg0);
    printf ("[M]   Mandatory arguments:\n");
    printf ("[M]     -op \t--output-preemption    \t<file>.csv \n");
    printf ("[M]     -on \t--output-no-preemption \t<file>.csv\n");
    printf ("[M]     -i  \t--input                \t<file>.dat\n");
    printf ("[M]     -q  \t--quantum              \t<positive value>\n");
}

void parse_cmdline (int argc, char **argv, char **i_filename, char **p_filename, char **n_filename, u_char *quantum){
    printf ("[M] Parsing arg list...\n");

	const char* const short_options = "o:i:q:";
    const struct option long_options[] = {
        {"output-preemption"   , 1, NULL, 'p'},
        {"output-no-preemption", 1, NULL, 'n'},
        {"input",                1, NULL, 'i'},
        {"quantum",              1, NULL, 'q'},
        {NULL,                   0, NULL,  0 }
    };
    int opt;
    char flags[4] = {0, 0, 0, 0};

    while ( (opt = getopt_long (argc, argv, short_options, long_options, NULL)) != -1) {
        switch (opt){
            case -1:
                break;
            case 'p':
                printf ("[M]   Option  P : %s\n", optarg);
                *p_filename = optarg;
                flags[0] = 1; break;
            case 'n':
                printf ("[M]   Option  N : %s\n", optarg);
                *n_filename = optarg;
                flags[1] = 1; break;
            case 'i':
                printf ("[M]   Option  I : %s\n", optarg);
                *i_filename = optarg;
                flags[2] = 1; break;
            case 'q':
                printf ("[M]   Option  Q : %s\n", optarg);
                *quantum = atoi (optarg);
                flags[3] = 1; break;
            case 'o':
                printf ("[M]   Option O-%c: %s\n", optarg[0]-32, argv[optind]);
                if (strlen (optarg)==1 && optind<argc){
                    if (strcmp (optarg, "p") == 0) {
                        *p_filename = argv[optind];
                        optind++; flags[0] = 1; break;
                    } else if (strcmp (optarg, "n") == 0) {
                        *n_filename = argv[optind];
                        optind++; flags[1] = 1; break;
                    }
                }
                /* Else falls through. */
            case '?':
            default:
				fprintf (stderr,"[M]   Bad parameter! (%c)\n", opt);
                print_help(argv[0]);
				exit(EX_USAGE);
        }
    }

    if (! (flags[0] && flags[1] && flags[2] && flags[3])){
        fprintf (stderr,"[M]   Missing parameters!\n");
        print_help(argv[0]);
		exit(EX_USAGE);
    }
}


u_long estimate_requirements (char *i_filename, u_int *proc_num, u_int *instr_num){
    printf ("[M] Estimating required shared memory...\n");

    FILE *data = fopen (i_filename, "r");
    if (data == NULL) {
        perror ("[M]   Failed to read input file");
        exit (EX_NOINPUT);
    }

    char c;
    while ( (c = (char)fgetc (data)) != EOF){
        switch (c){
            case 'j':
                *proc_num += 1; break;
            case 'i':
                *instr_num += 1; break;
        }
    }

    printf ("[M]   Proc size: %2d B,   Nodes needed: %7d,   Required memory: %8d B\n",
            size_of_proc, *proc_num, size_of_proc * *proc_num);
    printf ("[M]   Inst size: %2d B,   Nodes needed: %7d,   Required memory: %8d B\n",
            size_of_inst, *instr_num, size_of_inst * *instr_num);
    
    u_long required_mem = size_of_proc * *proc_num + size_of_inst * *instr_num;
    printf ("[M]   Total memory required: %lu bytes\n", required_mem);

    fclose (data);
    return required_mem;
}


void *setup_shared_mem (u_long required_mem){
    printf ("[M] Allocating shared memory...\n");

    int to_be_mapped = open ("/tmp/__RESERVED", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (to_be_mapped == -1) {
        perror ("[M]   Failed to create or truncate file");
        exit (EX_CANTCREAT);
    }

    lseek (to_be_mapped, required_mem+1, SEEK_SET);
    if (write (to_be_mapped, "", 1) != 1){
        perror ("[M]   Failed to write file");
        exit (EX_IOERR);
    };
    lseek (to_be_mapped, 0, SEEK_SET);

    void *shared_mem = mmap (0, required_mem, PROT_READ | PROT_WRITE, MAP_PRIVATE, to_be_mapped, 0);
    if (shared_mem == MAP_FAILED){
	    perror ("[M]   Failed to map shared mem");
        exit (EX_OSERR);
	}
    close (to_be_mapped);
    return shared_mem; 
}

void populate_shared_mem (void *shared_mem, char *i_filename){
    printf ("[M] Parsing %s and populating memory with structures...\n", i_filename);

    FILE *data = fopen (i_filename,"r");
    char *shm_cursor = shared_mem;
    char c;
    while ( (c = (char)fgetc (data)) != EOF){
        static process *prev_p = NULL, *p;
        static instruction *prev_i = NULL, *i;
        static int ret;
        switch (c){
            case 'j':
                p = (process*) shm_cursor;
                process_init (p);
                ret = fscanf (data, ",%hu,%u\n",
                             & (p->id), & (p->arrival_time));
                if (ret != 2){
                    fprintf (stderr, "[M]   Failed to parse input file.");
					exit(EX_DATAERR);
                }
                p->instruction_ptr = (instruction*) (shm_cursor + size_of_proc);
                if (prev_p != NULL)
                    prev_p->next_process = p;
                prev_p = p;
                shm_cursor += size_of_proc;
                prev_i = NULL;
                break;
            case 'i':
                i = (instruction*) shm_cursor;
                instruction_init (i);
                ret = fscanf (data, ",%d,%hhu,%hhu\n",
                             (int*)& (i->type), & (i->length), & (i->io_max));
                if (ret != 3){
                    fprintf (stderr, "[M]   Failed to parse input file.");
                    exit(EX_DATAERR);
                }
                p->instr_count += 1;
                p->total_cycles += i->length;
                if (prev_i != NULL)
                    prev_i->next_instruction = i;
                prev_i = i;
                shm_cursor += size_of_inst;
                break;
        }
    }

    fclose (data);
}

pid_t launch_scheduler (char label, void *shared_mem, u_int proc_num, char *outfile, u_char quantum, sched_policy sp, char core_n){

    pid_t pid = fork();

    if ( pid == 0 ) {
        printf ("[%c] Hello from the %c-scheduler!\n", label, label);
        scheduler (label, shared_mem, proc_num, outfile, quantum, sp, core_n);
        exit (EXIT_SUCCESS);
    }
    printf ("[M]   %c-scheduler started with PID %d\n", label, pid);
    return pid;
}

char wait_for_subprocesses(){
    printf ("[M] Waiting for childrens...\n");
	pid_t pid;
	char fail = 0;
    int status;
    while ( (pid = wait (&status)) ) {
		if (errno == ECHILD) break;
		if ( WIFEXITED(status) ) {
			if (WEXITSTATUS(status) == EXIT_SUCCESS){
				printf ("[M]   Child with PID %d exited successfully\n", pid);
			} else {
				printf ("[M]   Child with PID %d exited after a detected failure\n", pid);
			}
		} else {
			printf ("[M]   Child with PID %d aborted\n", pid);
			fail = 1;
		}
    }
    return fail;
}
