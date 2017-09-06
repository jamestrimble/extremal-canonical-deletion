all: ex_max_canonical_deletions

ex_max_canonical_deletions: ex_max_canonical_deletions.c util.c util.h graph_plus.h graph_plus.c graph_util.h graph_util.c possible_graph_types.c possible_graph_types.h
	gcc -O3 -g -ggdb -Wall -o ex_max_canonical_deletions graph_plus.c ex_max_canonical_deletions.c util.c graph_util.c possible_graph_types.c nautyL1.a
