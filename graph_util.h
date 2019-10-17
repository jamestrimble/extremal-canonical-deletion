#include <stdbool.h>

int nb_deg_sum(graph *g, int v, int *degs);

unsigned long long weighted_nb_nb_deg_sum(graph *g, int v, int *degs);

int num_neighbours_of_deg_d(graph *g, int v, int d, int *degs);

void all_pairs_check_for_short_path(graph *g, int n, int max_path_len, setword *have_short_path);

void extend_short_path_arr(graph *g, int n, int max_path_len, setword *have_short_path,
        setword *parent_have_short_path);

void show_graph(struct GraphPlus *gp);

void make_canonical(graph *g, int n, graph *canon_g);
