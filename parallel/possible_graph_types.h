#include <stdbool.h>

struct GraphType {
    struct GraphType *next;   // Next in hash-set chain
    int num_vertices;
    int num_edges;
    int min_deg;
    int max_deg;
};

void make_possible_graph_types(int n, int edge_count, int min_girth);

bool graph_type_is_in_set(struct GraphType *graph_type);

bool add_graph_type_to_set(struct GraphType *graph_type);

void clean_up_graph_type_lists();
