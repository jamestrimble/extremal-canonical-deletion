gcc -O3 -g -ggdb -pg -Wall -o ex_max_prof ex_max_canonical_deletions.c graph_plus.c graph_util.c util.c possible_graph_types.c nautyL1.a -lpthread
./ex_max_prof 5 32 85 | tail
gprof ex_max_prof gmon.out > prof_output
