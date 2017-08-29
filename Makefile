all: ex_max_canonical_deletions

ex_max_canonical_deletions: ex_max_canonical_deletions.c util.c util.h graph_plus2.h graph_plus2.c graph_util.h graph_util.c
	gcc -O3 -g -ggdb -Wall -o ex_max_canonical_deletions graph_plus2.c ex_max_canonical_deletions.c util.c graph_util.c nautyL1.a
