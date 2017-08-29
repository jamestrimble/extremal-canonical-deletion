gcc -O3 -g -ggdb -pg -Wall -o ex_max_prof ex_max_canonical_deletions.c graph_plus2.c graph_util.c expected_values.c util.c nautyL1.a
./ex_max_prof 5 32 85 | tail
gprof ex_max_prof gmon.out > prof_output
