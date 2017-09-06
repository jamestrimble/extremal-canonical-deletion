#include <stdbool.h>

int nb_deg_sum(graph *g, int v, int *degs);

unsigned long long weighted_nb_nb_deg_sum(graph *g, int v, int *degs);

int num_neighbours_of_deg_d(graph *g, int v, int d, int *degs);

void all_pairs_check_for_short_path(graph *g, int n, int max_path_len, setword *have_short_path);

bool graph_min_deg_geq_target(graph *g, int n, int target, int *degs);

bool graph_max_deg_eq_target(graph *g, int n, int target, int *degs);

void show_graph(struct GraphPlus *gp);
