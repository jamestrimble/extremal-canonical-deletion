#include "graph_plus.h"
#include "util.h"
#include "graph_util.h"
#include "possible_graph_types.h"

#include <stdbool.h>
#include <limits.h>

#define SPLITTING_ORDER 20

static int MIN_GIRTH;

static DEFAULTOPTIONS_GRAPH(options);
static statsblk stats;

static long long nauty_calls = 0;

int global_n;
int global_res = 0;
int global_mod = 0;

unsigned long long global_graph_count = 0;

void delete_neighbourhood(int v, graph *g)
{
    while (g[v]) {
        int nb;
        TAKEBIT(nb, g[v]);
        DELELEMENT(&g[nb], v);
    }
}

void make_canonical(graph *g, int n, graph *canon_g)
{
    int lab[MAXN],ptn[MAXN],orbits[MAXN];
    EMPTYGRAPH(canon_g,1,MAXN);
    densenauty(g,lab,ptn,orbits,&options,&stats,1,n,canon_g);
    nauty_calls++;
}

bool deletion_is_better(int v, graph *g, int n)
{
    graph g0[MAXN], g1[MAXN];
    for (int i=0; i<n; i++) {
        g0[i] = g[i];
        g1[i] = g[i];
    }
    // In g0, we delete vertex n-1 and its edges, and don't relabel vertices
    // In g1, we delete vertex v and its edges, and relabel vertex n-1 to v
    delete_neighbourhood(n-1, g0);
    delete_neighbourhood(n-1, g1);
    delete_neighbourhood(v, g1);
    setword neighbours = g[n-1];
    while (neighbours) {
        int nb;
        TAKEBIT(nb, neighbours);
        if (nb != v) {
            ADDONEEDGE(g1, v, nb, 1);
        }
    }

    graph g1_canon[MAXN];
    make_canonical(g1, n-1, g1_canon);

    return compare_graphs(hash_graph(g0, n-1),
                hash_graph(g1_canon, n-1),
                g0, g1_canon, n-1) == GREATER_THAN;
}

// For correctness, we have to be really careful about what rules we
// put in here.
// Assumption: the last vertex of g has degree equal to min_deg
bool deletion_is_canonical(graph *g, int n, int min_deg, int *degs) {
    int n0 = num_neighbours_of_deg_d(g, n-1, min_deg, degs);
    int nds0 = nb_deg_sum(g, n-1, degs);
    unsigned long long nnds0 = ULLONG_MAX;  // Only calculate this if we need it

    int vertices_to_check_deletion[MAXN];
    int vertices_to_check_deletion_len = 0;

    for (int i=0; i<n-1; i++) {
        if (degs[i]==min_deg) {
            int n1 = num_neighbours_of_deg_d(g, i, min_deg, degs);
            if (n1 > n0) {
                return false;
            } else if (n1 == n0) {
                int nds1 = nb_deg_sum(g, i, degs);
                if (nds1 > nds0) {
                    return false;
                } else if (nds1 == nds0) {
                    if (nnds0 == ULLONG_MAX)
                        nnds0 = weighted_nb_nb_deg_sum(g, n-1, degs);
                    unsigned long long nnds1 = weighted_nb_nb_deg_sum(g, i, degs);
                    if (nnds1 > nnds0) {
                        return false;
                    } else if (nnds1 == nnds0) {
                        // Delay the expensive checks that use Nauty;
                        // if we're lucky, we won't need to do them at all.
                        vertices_to_check_deletion[vertices_to_check_deletion_len++] = i;
                    }
                }
            }
        }
    }
    for (int i=0; i<vertices_to_check_deletion_len; i++)
        if (nnds0 && deletion_is_better(vertices_to_check_deletion[i], g, n))
            return false;

    return true;
}

// gp is the graph that we're augmenting
void output_graph(struct GraphPlus *gp, setword neighbours, bool max_deg_incremented,
        struct GraphPlusSet *gp_set)
{
    int n = gp->n + 1;

    graph new_g[MAXN];
    for (int i=0; i<MAXN; i++)
        new_g[i] = gp->graph[i];

    while (neighbours) {
        int nb;
        TAKEBIT(nb, neighbours);
        ADDONEEDGE(new_g, n-1, nb, 1);
    }

    int degs[MAXN];
    degs[n-1] = POPCOUNT(new_g[n-1]);
    for (int i=0; i<n-1; i++) {
        degs[i] = POPCOUNT(new_g[i]);
        if (degs[i] < degs[n-1])   // The last vertex must have minimum degree
            return;
    }

    if (deletion_is_canonical(new_g, n, degs[n-1], degs)) {
        graph new_g_canonical[MAXN];
        make_canonical(new_g, n, new_g_canonical);
        int edge_count = gp->edge_count + degs[n-1];
        int max_deg = gp->max_deg + max_deg_incremented;
        gp_set_add(gp_set, new_g_canonical, n, edge_count, degs[n-1], max_deg);
    }
}

// Arguments:
// gp:                       the graph we're trying to extend
// have_short_path:          is there a path of length <= MIN_GIRTH-3 from i to j?
// neighbours:               neighbours already chosen for the new vertex
// candidate_neighbours:     other neighbours that might be chosen for the new vertex
// gp_set:                     a pointer to set of new graphs that is being built
void search(struct GraphPlus *gp, setword *have_short_path,
        setword neighbours, setword candidate_neighbours, bool max_deg_incremented,
        struct GraphPlusSet *gp_set) 
{
    int neighbours_count = POPCOUNT(neighbours);

    if (graph_type_is_in_set(&(struct GraphType) {
                .num_vertices=gp->n+1,
                .num_edges=gp->edge_count+neighbours_count,
                .min_deg=neighbours_count,
                .max_deg=gp->max_deg+max_deg_incremented
            }))
        output_graph(gp, neighbours, max_deg_incremented, gp_set);

    if (neighbours_count == gp->min_deg + 1)
        return;

    while (candidate_neighbours) {
        int cand;
        TAKEBIT(cand, candidate_neighbours);
        ADDELEMENT(&neighbours, cand);
        setword new_candidates = candidate_neighbours & ~have_short_path[cand];
        search(gp, have_short_path, neighbours, new_candidates,
                max_deg_incremented || POPCOUNT(gp->graph[cand]) == gp->max_deg, gp_set);
        DELELEMENT(&neighbours, cand);
    }
}

void traverse_tree(struct GraphPlus *node,
        void (*callback)(struct GraphPlus *))
{
    if (node == NULL || callback == NULL)
        return;
    traverse_tree(node->left, callback);
    callback(node);
    traverse_tree(node->right, callback);
}

void visit_graph(struct GraphPlus *gp)
{
    if (gp->n==global_n) {
        // output graph
        global_graph_count++;
        show_graph(gp);
    } else {
        // add a vertex to the graph
        if (gp->n == SPLITTING_ORDER && global_mod!=0 && gp->hash%global_mod != global_res)
            return;

        setword have_short_path[MAXN];
        all_pairs_check_for_short_path(gp->graph, gp->n, MIN_GIRTH-3, have_short_path);

        setword candidate_neighbours = 0;
        for (int l=0; l<gp->n; l++)
            ADDELEMENT(&candidate_neighbours, l);

        struct GraphPlusSet gp_set = make_gp_set();
        search(gp, have_short_path, 0, candidate_neighbours, false, &gp_set);
        traverse_tree(gp_set.tree_head, visit_graph);
        free_tree(&gp_set.tree_head);
    }
}

void find_extremal_graphs(int n, int edge_count)
{
    if (global_mod > 0 && global_res > 0 && n <= SPLITTING_ORDER)
        return;

    make_possible_graph_types(n, edge_count, MIN_GIRTH);

    graph g[MAXN];
    EMPTYGRAPH(g,1,MAXN);
    struct GraphPlus gp;
    make_graph_plus(g, 1, 0, 0, 0, &gp);
    visit_graph(&gp);
}

int main(int argc, char *argv[])
{
    setlinebuf(stdout);

    options.getcanon = TRUE;
    options.tc_level = 0;

    if (argc < 4) {
        printf("Not enough arguments.\n");
        printf("Required: min girth, n, max edge count.\n");
        exit(1);
    }

    MIN_GIRTH = atoi(argv[1]);
    if (MIN_GIRTH < 5) {
        // This is because we use a result to restrict the min_deg
        // values we need to consider
        printf("Min girth must be >= 5\n");
        exit(1);
    }
    int n = atoi(argv[2]);
    int edge_count = atoi(argv[3]);
    if (argc > 4) {
        global_res = atoi(argv[4]);
        global_mod = atoi(argv[5]);
    }

    global_n = n;

    int m = SETWORDSNEEDED(n);
    if (m != 1) {
        printf("Unexpected value of m.\n");
        exit(1);
    }
    if (MAXM != 1) {
        printf("Unexpected value of MAXM.\n");
        exit(1);
    }

    nauty_check(WORDSIZE,m,n,NAUTYVERSIONID);

    find_extremal_graphs(n, edge_count);

    printf("Nauty calls: %lld\n", nauty_calls);
    printf("Total graph count: %llu\n", global_graph_count);

    clean_up_graph_type_lists();
}
