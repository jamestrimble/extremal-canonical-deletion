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

unsigned long long num_visited_by_order[MAXN] = {};

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

bool deletion_is_better(int v, graph *g, int n, int min_deg, int max_deg)
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

    setword sw0a = 0;
    setword sw1a = 0;
    for (int deg=min_deg; deg<=max_deg; deg++) {
        setword sw0 = 0;
        setword sw1 = 0;
        for (int i=0; i<n; i++) {
            if (POPCOUNT(g0[i]) == deg) {
                sw0 ^= g0[i];
                sw0a ^= g0[i];
            }
            if (POPCOUNT(g1[i]) == deg) {
                sw1 ^= g1[i];
                sw1a ^= g1[i];
            }
        }
        int pc0 = POPCOUNT(sw0);
        int pc1 = POPCOUNT(sw1);
        if (pc0 < pc1)
            return true;
        if (pc0 > pc1)
            return false;
        pc0 = POPCOUNT(sw0a);
        pc1 = POPCOUNT(sw1a);
        if (pc0 < pc1)
            return true;
        if (pc0 > pc1)
            return false;
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
bool deletion_is_canonical(graph *g, int n, int min_deg, int max_deg, int *degs,
        bool use_tentative_version) {
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
                    } else if (!use_tentative_version && nnds1 == nnds0) {
                        // Delay the expensive checks that use Nauty;
                        // if we're lucky, we won't need to do them at all.
                        vertices_to_check_deletion[vertices_to_check_deletion_len++] = i;
                    }
                }
            }
        }
    }
    for (int i=0; i<vertices_to_check_deletion_len; i++)
        if (deletion_is_better(vertices_to_check_deletion[i], g, n, min_deg, max_deg))
            return false;

    return true;
}

struct SearchData
{
    struct GraphPlus *gp;
    setword *have_short_path;
    struct GraphPlusSet *gp_set;
    setword min_degs[2];
    bool tentative_version;
    setword forced_neighbours;
    setword vertices_of_min_deg;
};

bool search(struct SearchData *sd,
        setword neighbours, setword candidate_neighbours, bool max_deg_incremented);

bool tentatively_visit_graph(struct GraphPlus *gp)
{
    if (gp->n==global_n)
        return true;

    setword min_degs[2];
    for (int i=0; i<2; i++) {
        struct GraphType * gt = find_graph_type_in_set(&(struct GraphType) {
                    .num_vertices=gp->n+1,
                    .num_edges_minus_min_deg=gp->edge_count,
                    .max_deg=gp->max_deg+i
                });
        min_degs[i] = gt ? gt->min_degs : 0;
    }

    setword candidate_neighbours = 0;
    setword vertices_of_min_deg = 0;
    setword combined_min_degs = min_degs[0] | min_degs[1];
    int must_increment_min_deg = true;
    for (int i=0; i<=gp->min_deg; i++) {
        if (ISELEMENT(&combined_min_degs, i)) {
            must_increment_min_deg = false;
            break;
        }
    }
    for (int l=0; l<gp->n; l++) {
        int pc = POPCOUNT(gp->graph[l]);
        if (pc==gp->max_deg && min_degs[1] == 0)
            continue;
        if (pc == gp->min_deg)
            ADDELEMENT(&vertices_of_min_deg, l);
        ADDELEMENT(&candidate_neighbours, l);
    }
    setword forced_neighbours = must_increment_min_deg ? vertices_of_min_deg : 0;

    if (POPCOUNT(forced_neighbours) > gp->min_deg + 1)
        return false;

    setword have_short_path[MAXN];
    all_pairs_check_for_short_path(gp->graph, gp->n, MIN_GIRTH-3, have_short_path);

    struct SearchData sd = {gp, have_short_path, NULL, {min_degs[0], min_degs[1]}, true, forced_neighbours,
            vertices_of_min_deg};
    return search(&sd, 0, candidate_neighbours, false);
}

// sd->gp is the graph that we're augmenting
bool output_graph(struct SearchData *sd, setword neighbours, bool max_deg_incremented)
{
    int n = sd->gp->n + 1;

    graph new_g[MAXN];
    for (int i=0; i<MAXN; i++)
        new_g[i] = sd->gp->graph[i];

    while (neighbours) {
        int nb;
        TAKEBIT(nb, neighbours);
        ADDONEEDGE(new_g, n-1, nb, 1);
    }

    int degs[MAXN];
    for (int i=0; i<n; i++)
        degs[i] = POPCOUNT(new_g[i]);

    int max_deg = sd->gp->max_deg + max_deg_incremented;
    if (sd->tentative_version) {
        return deletion_is_canonical(new_g, n, degs[n-1], max_deg, degs, true);
    } else if (deletion_is_canonical(new_g, n, degs[n-1], max_deg, degs, false)) {
        struct GraphPlus tentative_gp;
        int edge_count = sd->gp->edge_count + degs[n-1];
        make_graph_plus(new_g, n, edge_count, degs[n-1], max_deg, &tentative_gp);
        if (tentatively_visit_graph(&tentative_gp)) {
            graph new_g_canonical[MAXN];
            make_canonical(new_g, n, new_g_canonical);
            gp_set_add(sd->gp_set, new_g_canonical, n, edge_count, degs[n-1], max_deg);
            return true;
        }
    }
    return false;
}

// Arguments:
// sd->gp:                       the graph we're trying to extend
// sd->have_short_path:          is there a path of length <= MIN_GIRTH-3 from i to j?
// neighbours:               neighbours already chosen for the new vertex
// candidate_neighbours:     other neighbours that might be chosen for the new vertex
// sd->gp_set:                     a pointer to set of new graphs that is being built
// sd->min_degs[0]               the set of acceptable min degs if max degree is not incremented
// sd->min_degs[1]               the set of acceptable min degs if max degree is incremented
// max_deg_incremented           does the new graph have a higher max deg than the parent graph?
// sd->vertices_of_min_deg           which vertices in sd->gp have minimum degree?
bool search(struct SearchData *sd, setword neighbours, setword candidate_neighbours,
        bool max_deg_incremented)
{
    if (0 != (sd->forced_neighbours & ~(neighbours | candidate_neighbours)))
        return false;

    int neighbours_count = POPCOUNT(neighbours);

    if (ISELEMENT(&sd->min_degs[max_deg_incremented], neighbours_count) &&
            // the next line ensures that the new vertex has min degree
            (neighbours_count <= sd->gp->min_deg || (neighbours & sd->vertices_of_min_deg) == sd->vertices_of_min_deg) &&
            output_graph(sd, neighbours, max_deg_incremented) &&
            sd->tentative_version)
        return true;

    if (neighbours_count < sd->gp->min_deg + 1) {
        while (candidate_neighbours) {
            int cand;
            TAKEBIT(cand, candidate_neighbours);
            setword new_neighbours = neighbours | bit[cand];
            setword new_candidates = candidate_neighbours & ~sd->have_short_path[cand];
            if (search(sd, new_neighbours, new_candidates,
                    max_deg_incremented || POPCOUNT(sd->gp->graph[cand]) == sd->gp->max_deg) &&
                    sd->tentative_version)
                return true;
        }
    }
    return false;
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
    num_visited_by_order[gp->n]++;
    if (gp->n==global_n) {
        // output graph
        global_graph_count++;
        show_graph(gp);
    } else {
        // add a vertex to the graph
        if (gp->n == SPLITTING_ORDER && global_mod!=0 && gp->hash%global_mod != global_res)
            return;

        struct GraphPlusSet gp_set = make_gp_set();
        setword min_degs[2];
        for (int i=0; i<2; i++) {
            struct GraphType * gt = find_graph_type_in_set(&(struct GraphType) {
                        .num_vertices=gp->n+1,
                        .num_edges_minus_min_deg=gp->edge_count,
                        .max_deg=gp->max_deg+i
                    });
            min_degs[i] = gt ? gt->min_degs : 0;
        }

        setword candidate_neighbours = 0;
        setword vertices_of_min_deg = 0;
        setword combined_min_degs = min_degs[0] | min_degs[1];
        int must_increment_min_deg = true;
        for (int i=0; i<=gp->min_deg; i++) {
            if (ISELEMENT(&combined_min_degs, i)) {
                must_increment_min_deg = false;
                break;
            }
        }
        for (int l=0; l<gp->n; l++) {
            int pc = POPCOUNT(gp->graph[l]);
            if (pc==gp->max_deg && min_degs[1] == 0)
                continue;
            if (pc == gp->min_deg)
                ADDELEMENT(&vertices_of_min_deg, l);
            ADDELEMENT(&candidate_neighbours, l);
        }
        setword forced_neighbours = must_increment_min_deg ? vertices_of_min_deg : 0;

        setword have_short_path[MAXN];
        all_pairs_check_for_short_path(gp->graph, gp->n, MIN_GIRTH-3, have_short_path);

        struct SearchData sd = {gp, have_short_path, &gp_set, {min_degs[0], min_degs[1]}, false, forced_neighbours,
                vertices_of_min_deg};
        search(&sd, 0, candidate_neighbours, false);
//        printf("sz %lld\n", gp_set.sz);
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

    printf("visited");
    for (int i=0; i<MAXN; i++)
        printf(" %llu", num_visited_by_order[i]);
    printf("\n");

    printf("Nauty calls: %lld\n", nauty_calls);
    printf("Total graph count: %llu\n", global_graph_count);

    clean_up_graph_type_lists();
}
