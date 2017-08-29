#define MIN_DEG_UPPER_BOUND 8
#define MAX_DEG_UPPER_BOUND 63

#define MAX_SET_SIZE (1 << 19)

#include <stdbool.h>

#define MAXN 64   /* TODO: don't duplicate this */

#include "nauty.h"

enum comp {LESS_THAN, GREATER_THAN, EQUAL};

struct GraphPlus {
    setword hash;
//    unsigned long long invariant;
//    bool made_canonical;
    struct GraphPlus *left;   // left child in binary search tree
    struct GraphPlus *right;   // right child in binary search tree
    int n;
    graph graph[];
};

// TODO: stop making this do double duty as a list and as a set of possible augmentations
struct GraphPlusList {
    int num_vertices;
    int num_edges;
    int min_deg;
    int max_deg;
    unsigned long long sz;
    setword possible_augmentations_max_deg_same;
    setword possible_augmentations_max_deg_incremented;
    struct GraphPlus *tree_head;   // head of binary search tree
    struct GraphPlusList *next;
};

struct GraphPlusList *make_gp_list(int num_vertices, int num_edges, int min_deg, int max_deg);

struct GraphPlusList *get_or_make_gp_list(int num_vertices, int num_edges, int min_deg, int max_deg);

struct GraphPlusList *get_gp_list(int num_vertices, int num_edges, int min_deg, int max_deg);

int get_gp_list_sz(int num_vertices, int num_edges, int min_deg, int max_deg);

void clean_up_gp_lists();

void free_tree(struct GraphPlus **node_ptr);

void show_gp_list_sizes();

struct GraphPlus *alloc_graph_plus(int n);

setword hash_graph(graph *g, int n);

enum comp compare_graphs(setword graph_hash0, setword graph_hash1, graph *g0, graph *g1, int n);

void decompress_graph(graph *compressed_g, int n, graph *g);

// Returns pointer to new graph if it was added, or NULL if graph was in set already
struct GraphPlus * gp_list_add(struct GraphPlusList *list, graph *g, int n);

void show_graph(struct GraphPlus *gp, void *ignored);
