#ifndef GRAPH_PLUS_H
#define GRAPH_PLUS_H

#define MIN_DEG_UPPER_BOUND 8
#define MAX_DEG_UPPER_BOUND 63

#define MAX_SET_SIZE (1 << 19)

#include <stdbool.h>
#include <stdio.h>

#define MAXN 64

#include "nauty.h"

enum comp {LESS_THAN, GREATER_THAN, EQUAL};

struct GraphPlus {
    setword hash;
//    unsigned long long invariant;
    struct GraphPlus *left;   // left child in binary search tree
    struct GraphPlus *right;   // right child in binary search tree
    int n;
    int edge_count;
    int min_deg;
    int max_deg;
    graph graph[MAXN];
};

struct GraphPlusList {
    unsigned long long sz;
    struct GraphPlus *tree_head;   // head of binary search tree
};

struct GraphPlusList make_gp_list();

void free_tree(struct GraphPlus **node_ptr);

setword hash_graph(graph *g, int n);

enum comp compare_graphs(setword graph_hash0, setword graph_hash1, graph *g0, graph *g1, int n);

struct GraphPlus * make_graph_plus(graph *g, int n, int edge_count,
        int min_deg, int max_deg, struct GraphPlus *gp);

// Returns pointer to new graph if it was added, or NULL if graph was in set already
struct GraphPlus * gp_list_add(struct GraphPlusList *list, graph *g, int n, int edge_count, int min_deg, int max_deg);

#endif
