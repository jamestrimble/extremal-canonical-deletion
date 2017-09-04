/* This program ...

   This version uses a fixed limit for MAXN.
*/

#define MAXN 64    /* Define this before including nauty.h */
#include "nauty.h"   /* which includes <stdio.h> and other system files */
#include <stdbool.h>
#include <limits.h>
#include <time.h>

#include "util.h"
#include "graph_util.h"
#include "graph_plus.h"
#include "possible_augmentations.h"

#define MAX(a,b) ((a)>(b) ? (a) : (b))

#define SPLITTING_ORDER 21

// TODO: re-name lists to sets, where appropriate

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

bool min_and_max_deg_are_feasible(int n, int min_deg, int max_deg, int edge_count)
{
    if (max_deg == 0)
        return min_deg==0 && edge_count==0;

    if (min_deg*max_deg + 1 > n)
        return false;

    if (min_deg + (max_deg*(n-1)) < edge_count*2)
        return false;

    if (max_deg + (min_deg*(n-1)) > edge_count*2)
        return false;

    int tree_order = min_deg*max_deg+1;
    int extra_vv_required = 0;
    if (MIN_GIRTH >= 6) {
        // Taking advantage of the fact that if min girth is 6, then
        // there can be no extra edges added among vertices in the big star
        int tree_edge_count = tree_order-1;
        int extra_edges_required = edge_count - tree_edge_count;
        if (extra_edges_required<0) extra_edges_required=0;   //TODO: is this necessary?
        extra_vv_required =
            extra_edges_required/max_deg + (extra_edges_required%max_deg>0);
    }
    if (tree_order + extra_vv_required <= n) {
        return true;
    }

    return false;
}

bool ok_not_to_try_min_deg(int n, int min_deg, int edge_count)
{
    // lb is a lower bound on the number of vertices of degree min_deg in one of
    // the new graphs
    int lb = 0;
    while (lb < n && lb*min_deg + (n-lb)*(min_deg+1) > edge_count*2)
        lb++;

    // a is a lower bound on the number of edge endpoints that meet vertices
    // of degree min_deg
    int a = lb * min_deg;

    // If there must be two adjacent vertices of degree min_deg, then there
    // must be a way to make the graph by adding a vertex to a graph whose
    // vertex of minimum degree has degree min_deg-1
    return lb > 1 && a > edge_count;
}

void add_vertex(struct GraphPlus *gp);

void show_graph(struct GraphPlus *gp)
{
    global_graph_count++;
    printf("Graph with %d vertices\n", gp->n);
    for (int i=0; i<gp->n; i++) {
        for (int j=0; j<gp->n; j++) {
            printf("%s ", ISELEMENT(&gp->graph[i], j) ? "X" : ".");
        }
        printf("\n");
    }
    printf("\n");
}

// gp is the graph that we're augmenting
void output_graph(struct GraphPlus *gp, setword neighbours, bool max_deg_incremented,
        struct GraphPlusList *list)
{
    int n = gp->n + 1;
    while (neighbours) {
        int nb;
        TAKEBIT(nb, neighbours);
        ADDONEEDGE(gp->graph, n-1, nb, 1);
    }
    int degs[MAXN];
    degs[n-1] = POPCOUNT(gp->graph[n-1]);
    for (int i=0; i<n-1; i++) {
        degs[i] = POPCOUNT(gp->graph[i]);
        if (degs[i] < degs[n-1])   // The last vertex must have minimum degree
            goto clean_up_output_graph;
    }
    if (deletion_is_canonical(gp->graph, n, degs[n-1], degs)) {
        graph new_g[MAXN];
        make_canonical(gp->graph, n, new_g);
        int edge_count = gp->edge_count + degs[n-1];
        int max_deg = gp->max_deg + max_deg_incremented;
        gp_list_add(list, new_g, n, edge_count, degs[n-1], max_deg);
    }

clean_up_output_graph:
    // Delete the edges that were added
    while (gp->graph[n-1]) {
        int nb;
        TAKEBIT(nb, gp->graph[n-1]);
        DELELEMENT(&gp->graph[nb], n-1);
    }
}


// Arguments:
// gp:                       the graph we're trying to extend
// have_short_path:          is there a path of length <= MIN_GIRTH-3 from i to j?
// neighbours:               neighbours already chosen for the new vertex
// candidate_neighbours:     other neighbours that might be chosen for the new vertex
// list:                     a pointer to list of new graphs that is being built
void search(struct GraphPlus *gp, setword *have_short_path,
        setword neighbours, setword candidate_neighbours, bool max_deg_incremented,
        struct GraphPlusList *list) 
{
    int neighbours_count = POPCOUNT(neighbours);

    if (augmentation_is_in_set(&(struct Augmentation) {
                .num_vertices=gp->n,
                .num_edges=gp->edge_count,
                .min_deg=gp->min_deg,
                .max_deg=gp->max_deg,
                .new_vertex_deg=neighbours_count,
                .max_deg_incremented=max_deg_incremented
            }))
        output_graph(gp, neighbours, max_deg_incremented, list);

    if (neighbours_count == gp->min_deg + 1)
        return;

    while (candidate_neighbours) {
        int cand;
        TAKEBIT(cand, candidate_neighbours);
        ADDELEMENT(&neighbours, cand);
        setword new_candidates = candidate_neighbours & ~have_short_path[cand];
        search(gp, have_short_path, neighbours, new_candidates,
                max_deg_incremented || POPCOUNT(gp->graph[cand]) == gp->max_deg, list);
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
        show_graph(gp);
    } else {
        add_vertex(gp);
    }
}

void add_vertex(struct GraphPlus *gp)
{
    if (gp->n == SPLITTING_ORDER && global_mod!=0 && gp->hash%global_mod != global_res)
        return;

    setword have_short_path[MAXN];
    all_pairs_check_for_short_path(gp->graph, gp->n, MIN_GIRTH-3, have_short_path);

    setword candidate_neighbours = 0;
    for (int l=0; l<gp->n; l++)
        ADDELEMENT(&candidate_neighbours, l);

    struct GraphPlusList list = make_gp_list();
    search(gp, have_short_path, 0, candidate_neighbours, false, &list);
    traverse_tree(list.tree_head, visit_graph);
    free_tree(&list.tree_head);
}

void make_possible_augmentations_recurse(int n, int edge_count, int min_deg, int max_deg)
{
    if (n == 1)
        return;
    
    int small_g_ec = edge_count - min_deg;
    if (small_g_ec >= 0) {
        // i is min deg in the graph with one vertex fewer
        // j is max deg in the graph with one vertex fewer
        int start_i = MAX(min_deg-1, 0);
        for (int i=start_i; i<=MIN_DEG_UPPER_BOUND; i++) {
            if (i==min_deg && ok_not_to_try_min_deg(n, min_deg, edge_count))
                continue;
            int start_j = MAX(i, max_deg-1);
            for (int j=start_j; j<=max_deg; j++) {
                if (min_and_max_deg_are_feasible(n-1, i, j, small_g_ec)) {
                    struct Augmentation aug = {
                                .num_vertices=n-1,
                                .num_edges=small_g_ec,
                                .min_deg=i,
                                .max_deg=j,
                                .new_vertex_deg=min_deg,
                                .max_deg_incremented=(j==max_deg-1)
                            };
                    if (add_augmentation_to_set(&aug)) {
                        make_possible_augmentations_recurse(n-1, small_g_ec, i, j);
                    }
                }
            }
        }
    }
}

void make_possible_augmentations(int n, int edge_count)
{
    for (int min_deg=0; min_deg<=MIN_DEG_UPPER_BOUND; min_deg++) {
        for (int max_deg=min_deg; max_deg<=MAX_DEG_UPPER_BOUND; max_deg++) {
            if (min_and_max_deg_are_feasible(n, min_deg, max_deg, edge_count)) {
                make_possible_augmentations_recurse(n, edge_count, min_deg, max_deg);
            }
        }
    }
}

void find_extremal_graphs(int n, int edge_count, clock_t start_time)
{
    if (global_mod > 0 && global_res > 0 && n <= SPLITTING_ORDER)
        return;

    make_possible_augmentations(n, edge_count);

    graph g[MAXN];
    EMPTYGRAPH(g,1,MAXN);
    struct GraphPlus gp;
    make_graph_plus(g, 1, 0, 0, 0, &gp);
    add_vertex(&gp);
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

    clock_t start_time = clock();

    find_extremal_graphs(n, edge_count, start_time);

    printf("Nauty calls: %lld\n", nauty_calls);
    printf("Total graph count: %llu\n", global_graph_count);

    clean_up_augmentation_lists();
}
