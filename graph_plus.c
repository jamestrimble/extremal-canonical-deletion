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

static struct GraphPlus *alloc_graph_plus(int n) {
    return emalloc(sizeof(struct GraphPlus));
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

static void compress_graph(graph *g, int n, graph *compressed_g)
{
    for (int i=0; i<n; i++)
        compressed_g[i] = g[i];
}

//static unsigned long long calc_invariant(graph *g, int n)
//{
//    return 0;
//    unsigned long long invar = 0ull;
//    int degs[MAXN];
//    for (int i=0; i<n; i++)
//        degs[i] = POPCOUNT(g[i]);
//
//    for (int i=0; i<n; i++) {
//        setword neighbourhood = g[i];
//        int w;
//        while (neighbourhood) {
//            TAKEBIT(w, neighbourhood);
//            invar += degs[i] * degs[w] * degs[w];
//        }
//    }
//    invar = (invar ^ (invar >> 30)) * 0xbf58476d1ce4e5b9ull;
//    invar = (invar ^ (invar >> 27)) * 0x94d049bb133111ebull;
//    invar = invar ^ (invar >> 31);
//    return invar;
//}

static struct GraphPlus * make_graph_plus(graph *g, int n, struct GraphPlus *gp) {
    gp->left = NULL;
    gp->right = NULL;
    gp->n = n;
    gp->hash = hash_graph(g, n);
    compress_graph(g, n, gp->graph);
    return gp;
}

// Returns pointer to new graph if it was added, or NULL if graph was in set already
struct GraphPlus * gp_list_add(struct GraphPlusList *list, graph *g, int n)
{
    struct GraphPlus **root = &list->tree_head;
    setword g_hash = hash_graph(g, n);   // TODO: reduce duplicate work in calling hash_graph multiple times?
    while (*root) {
        switch (compare_graphs(g_hash, (*root)->hash, g, (*root)->graph, n)) {
            case LESS_THAN: root = &((*root)->left);  break;
            case GREATER_THAN: root = &((*root)->right); break;
            default: return NULL;    // the element is in the list
        }
    }
    struct GraphPlus *gp = alloc_graph_plus(n);
    make_graph_plus(g, n, gp);
    *root = gp;
    list->sz++;
    return gp;
}

void show_graph(struct GraphPlus *gp, void *ignored)
{
    printf("Graph with %d vertices\n", gp->n);
    for (int i=0; i<gp->n; i++) {
        for (int j=0; j<gp->n; j++) {
            printf("%s ", ISELEMENT(&gp->graph[i], j) ? "X" : ".");
        }
        printf("\n");
    }
    printf("\n");
}

