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

#define SPLITTING_ORDER 20 

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

//static unsigned long long calc_invariant(graph *g, int n)
//{
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
//    return invar;
//}

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

//    unsigned long long invar0 = calc_invariant(g0, n-1);
//    unsigned long long invar1 = calc_invariant(g1, n-1);
//    if (invar0 < invar1)
//        return true;
//    if (invar0 > invar1)
//        return false;

    int lab[MAXN],ptn[MAXN],orbits[MAXN];
    graph g1_canon[MAXN];
    EMPTYGRAPH(g1_canon,1,MAXN);
    densenauty(g1,lab,ptn,orbits,&options,&stats,1,n-1,g1_canon);
    nauty_calls++;

    return compare_graphs(hash_graph(g0, n-1),
                hash_graph(g1_canon, n-1),
                g0, g1_canon, n-1) == GREATER_THAN;
}

// For correctness, we have to be really careful about what rules we
// put in here.
// Tie-break by sum of neighbours' neighbours' degrees
// Assumption: the last vertex of g has degree equal to min_deg
bool deletion_is_canonical(graph *g, int n, int min_deg, int *degs) {
    int n0 = num_neighbours_of_deg_d(g, n-1, min_deg, degs);
    int nds0 = nb_deg_sum(g, n-1, degs);
    int nnds0 = -1;  // Only calculate this if we need it
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
                    if (nnds0 == -1)
                        nnds0 = nb_nb_deg_sum(g, n-1, degs);
                    int nnds1 = nb_nb_deg_sum(g, i, degs);
                    if (nnds1 > nnds0) {
                        return false;
                    } else if (nnds1 == nnds0 && deletion_is_better(i, g, n)) {
                        return false;
                    }
                }
            }
        }
    }
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

void add_vertex(graph *g, int g_n, int g_edge_count, int g_min_deg, int g_max_deg);

void show_graph2(struct GraphPlus *gp)
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

int get_max_deg(graph *g, int n, int *degs)
{
    int max_deg = 0;
    for (int i=0; i<n; i++)
        if (degs[i] > max_deg)
            max_deg = degs[i];
    return max_deg;
}

bool graph_last_vtx_has_min_deg(graph *g, int n, int *degs) {
    for (int i=0; i<n-1; i++)
        if (degs[i] < degs[n-1])
            return false;
    return true;
}

void output_graph2(graph *g, int n, setword neighbours, struct GraphPlusList *list)
{
    while (neighbours) {
        int nb;
        TAKEBIT(nb, neighbours);
        ADDONEEDGE(g, n-1, nb, 1);
    }
    int degs[MAXN];
    unsigned edge_endpoint_count = 0;
    for (int i=0; i<n; i++) {
        degs[i] = POPCOUNT(g[i]);
        edge_endpoint_count += degs[i];
    }
    int edge_count = edge_endpoint_count >> 1; // TODO: avoid having to calculate this like this?
    int max_deg = get_max_deg(g, n, degs);
    if (graph_last_vtx_has_min_deg(g, n, degs) &&
            deletion_is_canonical(g, n, degs[n-1], degs)) {
        graph new_g[MAXN];   // make a copy, since gp_list_add may relabel the graph
        EMPTYGRAPH(new_g,1,MAXN);
        int lab[MAXN],ptn[MAXN],orbits[MAXN];
        nauty_calls++;
        densenauty(g,lab,ptn,orbits,&options,&stats,1,n,new_g);
        struct GraphPlus *gp = gp_list_add(list, new_g, n);
        if (gp) {
            if (n==global_n) {
                show_graph2(gp);
            } else {
                add_vertex(new_g, n, edge_count, degs[n-1], max_deg);
            }
        }
    }

    // Delete the edges that were added
    while (g[n-1]) {
        int nb;
        TAKEBIT(nb, g[n-1]);
        DELELEMENT(&g[nb], n-1);
    }
}


// Arguments:
// g:                        the graph we're trying to extend
// g_min_deg:                the min degree of g
// n:                        degree of new graph
// have_short_path:          is there a path of length <= MIN_GIRTH-3 from i to j?
// neighbours:               neighbours already chosen for the new vertex
// candidate_neighbours:     other neighbours that might be chosen for the new vertex
// list:                     a pointer to list of new graphs that is being built
void search2(graph *g, int g_edge_count, int g_min_deg, int g_max_deg, int n,
        setword *have_short_path,
        setword neighbours, setword candidate_neighbours, bool max_deg_incremented,
        struct GraphPlusList *list) 
{
    int neighbours_count = POPCOUNT(neighbours);

    if (neighbours_count > g_min_deg + 1)
        return;

    if (augmentation_is_in_set((struct Augmentation) {
                .num_vertices=n-1,
                .num_edges=g_edge_count,
                .min_deg=g_min_deg,
                .max_deg=g_max_deg,
                .new_vertex_deg=neighbours_count,
                .max_deg_incremented=max_deg_incremented
            }))
        output_graph2(g, n, neighbours, list);

    while (candidate_neighbours) {
        int cand;
        TAKEBIT(cand, candidate_neighbours);
        ADDELEMENT(&neighbours, cand);
        if (!max_deg_incremented && POPCOUNT(g[cand]) == g_max_deg)
            max_deg_incremented = true;
        setword new_candidates = candidate_neighbours & ~have_short_path[cand];
        search2(g, g_edge_count, g_min_deg, g_max_deg, n, have_short_path, neighbours, new_candidates, max_deg_incremented, list);
        DELELEMENT(&neighbours, cand);
    }
}

void add_vertex(graph *g, int g_n, int g_edge_count, int g_min_deg, int g_max_deg)
{
    if (g_n == SPLITTING_ORDER && global_mod!=0 && hash_graph(g, g_n)%global_mod != global_res)
        return;

    setword have_short_path[MAXN];
    all_pairs_check_for_short_path(g, g_n, MIN_GIRTH-3, have_short_path);

    setword candidate_neighbours = 0;
    for (int l=0; l<g_n; l++)
        ADDELEMENT(&candidate_neighbours, l);

    struct GraphPlusList list = make_gp_list();
    search2(g, g_edge_count, g_min_deg, g_max_deg, g_n+1, have_short_path, 0, candidate_neighbours, false, &list);
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
                    if (add_augmentation_to_set(aug)) {
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
    add_vertex(g, 1, 0, 0, 0);
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
