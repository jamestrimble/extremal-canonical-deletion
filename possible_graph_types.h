#include <stdbool.h>

struct GraphType {
    struct GraphType *next;   // Next in hash-set chain
    int num_vertices;
    int num_edges_minus_min_deg;
    setword min_degs;
    int max_deg;
    setword lb_on_num_vv_of_min_deg_tried[MAXN];
};

void make_possible_graph_types(int n, int edge_count, int min_girth);

struct GraphType * find_graph_type_in_set(struct GraphType *graph_type);

bool add_graph_type_to_set(struct GraphType *graph_type, int min_deg, int lb_on_num_vv_of_min_deg);

void clean_up_graph_type_lists();
