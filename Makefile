# Flags for a standard build
CC = gcc
CFLAGS = -Wall -Wextra -pedantic -march=native
LDFLAGS = -pthread

# Flags for recipe-specific behaviour
PRDFLAGS = -Ofast -s -fno-stack-protector -D NDEBUG -flto
DBGFLAGS = -ggdb3 -D DBG_VTE
MTRFLAGS = -pg -fprofile-arcs -ftest-coverage -fno-inline -D NDEBUG

# Useful project-related variables
OBJECTS = master.o scheduler.o core.o datastructs_helpers.o scheduler_helpers.o
EXENAME = simulator

# Main recipes, each one has specific additional flags added to CFLAGS
# Note: custom CFLAGS propagates to sub-recipes (%.o in this case)
## all = release;
## dbg = debug, it triggers the precompiler to wire the code in
##       order to provide two gnome-terminal instances piped to extra
##       debug information output;
## mtr = metering, generate informations useful for "gcov/gprof" tools;
all: CFLAGS += $(PRDFLAGS)
all: obj
	$(CC) $(OBJECTS) $(CFLAGS) $(LDFLAGS) -o $(EXENAME)

dbg: CFLAGS += $(DBGFLAGS)
dbg: obj
	$(CC) $(OBJECTS) $(CFLAGS) $(LDFLAGS) -o $(EXENAME)_dbg

mtr: CFLAGS += $(MTRFLAGS)
mtr: obj
	$(CC) $(OBJECTS) $(CFLAGS) $(LDFLAGS) -o $(EXENAME)_mtr

# Allows generation of object files only.
obj: $(OBJECTS)

# Pattern rule for object file generation.
%.o: %.c
	$(CC) $< -c $(CFLAGS) -o $@

clean: clean_bin clean_obj clean_cov clean_csv clean_prf

clean_bin:
	rm -f $(EXENAME) $(EXENAME)_dbg $(EXENAME)_mtr

clean_obj:
	rm -f *.o

clean_cov:
	rm -f *.gcov *.gcda *.gcno

clean_csv:
	rm -f *.csv

clean_prf:
	rm -f gmon.out

# Not used, but can be used in %.o rule as a requirement. An empty rule
# like this can force sub-recipes to recompile objects.
# For example, by issuing "make all" first and then "make dbg", the
# debug version will be ill-formed, because the second recipe linked
# the executable using object files form the previous recipe call.
# You would need a "make clean" in between, or force recompilation every
# time by tricking Make with an empty target like "FORCE" among the
# requirements of the %.o sub-recipe
FORCE:

