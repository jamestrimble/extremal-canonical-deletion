all: ex_max_canonical_deletions ex_max_canonical_deletions_almost_self_contained

ex_max_canonical_deletions: ex_max_canonical_deletions.c util.c util.h graph_plus.h graph_plus.c graph_util.h graph_util.c possible_graph_types.c possible_graph_types.h
	gcc -O3 -march=native -g -ggdb -Wall -o ex_max_canonical_deletions graph_plus.c ex_max_canonical_deletions.c util.c graph_util.c possible_graph_types.c nautyL1.a -mpopcnt

ex_max_canonical_deletions_almost_self_contained: ex_max_canonical_deletions.c util.c util.h graph_plus.h graph_plus.c graph_util.h graph_util.c possible_graph_types.c possible_graph_types.h
	gcc -DSELF_CONTAINED -O3 -march=native -g -ggdb -Wall -o ex_max_canonical_deletions_almost_self_contained graph_plus.c ex_max_canonical_deletions.c util.c graph_util.c possible_graph_types.c nautyL1.a -mpopcnt

clean:
	rm -f ex_max_canonical_deletions ex_max_canonical_deletions_almost_self_contained
