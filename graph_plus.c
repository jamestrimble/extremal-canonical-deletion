#include "graph_plus.h"
#include "util.h"

struct GraphPlusSet make_gp_set()
{
    return (struct GraphPlusSet) {.sz=0, .tree_head=0};
}

void free_tree(struct GraphPlus **node_ptr)
{
    struct GraphPlus *node = *node_ptr;
    if (node == NULL)
        return;
    free_tree(&node->left);
    free_tree(&node->right);
    free(node);
    *node_ptr = NULL;
}

enum comp compare_graphs(graph *g0, graph *g1, int n)
{
    for (int i=0; i<n; i++) {
        if (g0[i] < g1[i]) return LESS_THAN;
        if (g0[i] > g1[i]) return GREATER_THAN;
    }
    return EQUAL;
}

struct GraphPlus * make_graph_plus(graph *g, int n, int edge_count,
        int min_deg, int max_deg, struct GraphPlus *gp) {
    gp->left = NULL;
    gp->right = NULL;
    gp->n = n;
    gp->edge_count = edge_count;
    gp->min_deg = min_deg;
    gp->max_deg = max_deg;
    for (int i=0; i<n; i++)
        gp->graph[i] = g[i];
    for (int i=n; i<MAXN; i++)
        gp->graph[i] = 0;
    return gp;
}

// Returns pointer to new graph if it was added, or NULL if graph was in set already
struct GraphPlus * gp_set_add(struct GraphPlusSet *gp_set, graph *g, int n, int edge_count, int min_deg, int max_deg)
{
    struct GraphPlus **root = &gp_set->tree_head;
    struct GraphPlus *gp = emalloc(sizeof(struct GraphPlus));
    make_graph_plus(g, n, edge_count, min_deg, max_deg, gp);
    while (*root) {
        switch (compare_graphs(g, (*root)->graph, n)) {
            case LESS_THAN: root = &((*root)->left);  break;
            case GREATER_THAN: root = &((*root)->right); break;
            default: free(gp); return NULL;    // the element is in the set
        }
    }
    *root = gp;
    gp_set->sz++;
    return gp;
}
