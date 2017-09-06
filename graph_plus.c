#include "graph_plus.h"
#include "util.h"

struct GraphPlusList make_gp_list()
{
    return (struct GraphPlusList) {.sz=0, .tree_head=0};
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

setword hash_graph(graph *g, int n) {
    setword hash = 0ull;
    for (int i=0; i<n; i++)
        hash ^= g[i]*(i+1);
    // Try to get a good hash in order to keep the treesets reasonably well balanced
    // https://stackoverflow.com/a/12996028
    hash = (hash ^ (hash >> 30)) * 0xbf58476d1ce4e5b9ull;
    hash = (hash ^ (hash >> 27)) * 0x94d049bb133111ebull;
    hash = hash ^ (hash >> 31);
    return hash;
}

enum comp compare_graphs(setword graph_hash0, setword graph_hash1, graph *g0, graph *g1, int n)
{
    if (graph_hash0 < graph_hash1) return LESS_THAN;
    if (graph_hash0 > graph_hash1) return GREATER_THAN;
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
    gp->hash = hash_graph(g, n);
    for (int i=0; i<n; i++)
        gp->graph[i] = g[i];
    for (int i=n; i<MAXN; i++)
        gp->graph[i] = 0;
    return gp;
}

// Returns pointer to new graph if it was added, or NULL if graph was in set already
struct GraphPlus * gp_list_add(struct GraphPlusList *list, graph *g, int n, int edge_count, int min_deg, int max_deg)
{
    struct GraphPlus **root = &list->tree_head;
    struct GraphPlus *gp = emalloc(sizeof(struct GraphPlus));
    make_graph_plus(g, n, edge_count, min_deg, max_deg, gp);
    while (*root) {
        switch (compare_graphs(gp->hash, (*root)->hash, g, (*root)->graph, n)) {
            case LESS_THAN: root = &((*root)->left);  break;
            case GREATER_THAN: root = &((*root)->right); break;
            default: free(gp); return NULL;    // the element is in the list
        }
    }
    *root = gp;
    list->sz++;
    return gp;
}
