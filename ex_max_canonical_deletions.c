#include "graph_plus.h"
#include "util.h"
#include "graph_util.h"
#include "possible_graph_types.h"

#include <stdbool.h>
#include <limits.h>

#define MAX_TENTATIVENESS_LEVEL 3

static int MIN_GIRTH;

static long long canonicalisation_calls = 0;

int global_n;
int global_low_splitting_level = 0;
int global_high_splitting_level = 0;
int global_split_number = 0;

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

bool deletion_is_better(int v, graph *g, int n, int min_deg, int max_deg, int tentativeness_level)
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

    if (tentativeness_level != 0)
        return false;

    graph g1_canon[MAXN];
    make_canonical(g1, n-1, g1_canon);
    canonicalisation_calls++;

    return compare_graphs(hash_graph(g0, n-1),
                hash_graph(g1_canon, n-1),
                g0, g1_canon, n-1) == GREATER_THAN;
}

// For correctness, we have to be really careful about what rules we
// put in here.
// Assumption: the last vertex of g has degree equal to min_deg
bool deletion_is_canonical(graph *g, int n, int min_deg, int max_deg,
        int tentativeness_level, setword vertices_of_min_deg)
{
    int n0 = POPCOUNT(g[n-1] & vertices_of_min_deg);
    int nds0mod = modified_nb_deg_sum(g, n-1, vertices_of_min_deg);

    int vertices_to_check_deletion[MAXN];
    int vertices_to_check_deletion_len = 0;

    setword vv = vertices_of_min_deg ^ bit[n-1];
    while (vv) {
        int i;
        TAKEBIT(i, vv);

        int n1 = POPCOUNT(g[i] & vertices_of_min_deg);
        if (n1 > n0)
            return false;
        else if (n1 != n0)
            continue;

        int nds1mod = modified_nb_deg_sum(g, i, vertices_of_min_deg);
        if (nds1mod < nds0mod)
            return false;
        else if (nds1mod != nds0mod)
            continue;

        vertices_to_check_deletion[vertices_to_check_deletion_len++] = i;
    }

    if (vertices_to_check_deletion_len == 0)
        return true;

    int j = 0;
    int nds0 = nb_deg_sum_self_contained(g, n-1);
    for (int i=0; i<vertices_to_check_deletion_len; i++) {
        int v = vertices_to_check_deletion[i];
        int nds1 = nb_deg_sum_self_contained(g, v);
        if (nds1 < nds0)
            return false;
        else if (nds1 == nds0)
            vertices_to_check_deletion[j++] = v;
    }
    vertices_to_check_deletion_len = j;

    if (vertices_to_check_deletion_len == 0)
        return true;

    unsigned long long nnds0 = weighted_nb_nb_deg_sum(g, n-1);
    j = 0;
    for (int i=0; i<vertices_to_check_deletion_len; i++) {
        int v = vertices_to_check_deletion[i];
        unsigned long long nnds1 = weighted_nb_nb_deg_sum(g, v);
        if (nnds1 < nnds0) {
            return false;
        } else if (nnds1 == nnds0) {
            // Delay the expensive checks that use Nauty;
            // if we're lucky, we won't need to do them at all.
            vertices_to_check_deletion[j++] = v;
        }
    }
    vertices_to_check_deletion_len = j;

    for (int i=0; i<vertices_to_check_deletion_len; i++)
        if (deletion_is_better(vertices_to_check_deletion[i], g, n, min_deg, max_deg, tentativeness_level))
            return false;

    return true;
}

struct SearchData
{
    struct GraphPlus *gp;
    setword *have_short_path;
    struct GraphPlusSet *gp_set;
    setword min_degs[2];
    int tentativeness_level;
    setword vertices_of_min_deg;
    setword vertices_of_min_deg_plus1;
};

bool visit_graph(struct GraphPlus *gp, int tentativeness_level, graph *parent_have_short_path);

// sd->gp is the graph that we're augmenting
bool output_graph(struct SearchData *sd, setword neighbours, bool max_deg_incremented)
{
    int n = sd->gp->n + 1;

    graph new_g[MAXN];
    for (int i=0; i<MAXN; i++)
        new_g[i] = sd->gp->graph[i];

    setword neighbours_copy = neighbours;
    while (neighbours_copy) {
        int nb;
        TAKEBIT(nb, neighbours_copy);
        new_g[nb] |= bit[n-1];
    }
    new_g[n-1] = neighbours;

    int min_deg = POPCOUNT(new_g[n-1]);
    int max_deg = sd->gp->max_deg + max_deg_incremented;

    setword vertices_of_min_deg;
    if (min_deg == sd->gp->min_deg) {
        vertices_of_min_deg = bit[n-1] | (sd->vertices_of_min_deg & ~neighbours);
    } else if (min_deg > sd->gp->min_deg) {
        vertices_of_min_deg = bit[n-1] | sd->vertices_of_min_deg | (sd->vertices_of_min_deg_plus1 & ~neighbours);
    } else {
        vertices_of_min_deg = bit[n-1];
    }
    if (!deletion_is_canonical(new_g, n, min_deg, max_deg, sd->tentativeness_level,
            vertices_of_min_deg))
        return false;

    struct GraphPlus tentative_gp;
    int edge_count = sd->gp->edge_count + min_deg;
    make_graph_plus(new_g, n, edge_count, min_deg, max_deg, &tentative_gp);
    if (!visit_graph(&tentative_gp, sd->tentativeness_level + 1, sd->have_short_path))
        return false;

    if (sd->tentativeness_level == 0) {
        graph new_g_canonical[MAXN];
        make_canonical(new_g, n, new_g_canonical);
        canonicalisation_calls++;
        struct GraphPlus *canonicalised_gp = gp_set_add(
                sd->gp_set, new_g_canonical, n, edge_count, min_deg, max_deg);
        if (canonicalised_gp)   // if not already in set
            visit_graph(canonicalised_gp, 0, NULL);
    }
    return true;
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
    int neighbours_count = POPCOUNT(neighbours);

    if (ISELEMENT(&sd->min_degs[max_deg_incremented], neighbours_count) &&
            // the next line ensures that the new vertex has min degree
            (neighbours_count <= sd->gp->min_deg || (neighbours & sd->vertices_of_min_deg) == sd->vertices_of_min_deg) &&
            output_graph(sd, neighbours, max_deg_incremented) &&
            sd->tentativeness_level)
        return true;

    if (neighbours_count <= sd->gp->min_deg) {
        while (candidate_neighbours) {
            int cand;
            TAKEBIT(cand, candidate_neighbours);
            setword new_neighbours = neighbours | bit[cand];
            setword new_candidates = candidate_neighbours & ~sd->have_short_path[cand];
            if (search(sd, new_neighbours, new_candidates,
                    max_deg_incremented || POPCOUNT(sd->gp->graph[cand]) == sd->gp->max_deg) &&
                    sd->tentativeness_level)
                return true;
        }
    }
    return false;
}

// add a vertex to the graph
bool visit_graph(struct GraphPlus *gp, int tentativeness_level, graph *parent_have_short_path)
{
    if (!tentativeness_level) {
        num_visited_by_order[gp->n]++;

        if (gp->n==global_n) {
            // output graph
            global_graph_count++;
            show_graph(gp);
            return true;        // return value of non-tentative version is unused
        }

        if (global_n > global_high_splitting_level &&
                gp->n >= global_low_splitting_level &&
                gp->n <= global_high_splitting_level) {
            bool bit = (num_visited_by_order[gp->n] & 1ull) != 0;
            if ((((unsigned) global_split_number >> (gp->n - global_low_splitting_level)) & 1) == bit)
                return true;
        }
    } else if (gp->n == global_n) {
        return true;
    }

    struct GraphPlusSet gp_set;
    struct GraphPlusSet *gp_set_ptr = NULL;
    if (!tentativeness_level) {
        gp_set = make_gp_set();
        gp_set_ptr = &gp_set;
    }

    setword min_degs[2];
    for (int i=0; i<2; i++) {
        struct GraphType * gt = find_graph_type_in_set(&(struct GraphType) {
                    .num_vertices=gp->n+1,
                    .num_edges_minus_min_deg=gp->edge_count,
                    .max_deg=gp->max_deg+i
                });
        min_degs[i] = gt ? gt->min_degs : 0;
    }

    setword bits_up_to_min_deg = ALLMASK(gp->min_deg + 1);
    bool must_increment_min_deg = 0 == (bits_up_to_min_deg & (min_degs[0] | min_degs[1]));

    setword vertices_of_deg[MAXN];
    for (int i=gp->min_deg; i<=gp->max_deg; i++)
        vertices_of_deg[i] = 0;
    for (int i=0; i<gp->n; i++)
        vertices_of_deg[POPCOUNT(gp->graph[i])] |= bit[i];
    setword vertices_of_min_deg = vertices_of_deg[gp->min_deg];
    setword vertices_of_min_deg_plus1 =vertices_of_deg[gp->min_deg+1];
    setword vertices_of_max_deg =vertices_of_deg[gp->max_deg];

    setword forced_neighbours = must_increment_min_deg ? vertices_of_min_deg : 0;

    setword candidate_neighbours = ALLMASK(gp->n);
    if (min_degs[1] == 0)
        candidate_neighbours &= ~vertices_of_max_deg;

    if (POPCOUNT(forced_neighbours) > gp->min_deg + 1)
        return false;

    if (tentativeness_level == MAX_TENTATIVENESS_LEVEL || (tentativeness_level != 0 && gp->n == global_n))
        return true;

    setword have_short_path[MAXN];
    if (!tentativeness_level) {
        all_pairs_check_for_short_path(gp->graph, gp->n, MIN_GIRTH-3, have_short_path);
    } else {
        extend_short_path_arr(gp->graph, gp->n, MIN_GIRTH-3, have_short_path, parent_have_short_path);
    }

    setword neighbours = 0;
    bool max_deg_incremented = false;
    if (forced_neighbours) {
        setword forced_neighbours_copy = forced_neighbours;
        bool all_forced_neighbours_are_feasible = true;
        while (forced_neighbours_copy) {
            int cand;
            TAKEBIT(cand, forced_neighbours_copy);
            ADDELEMENT(&neighbours, cand);
            candidate_neighbours &= ~have_short_path[cand];
            all_forced_neighbours_are_feasible &= 0 == (forced_neighbours & ~(neighbours | candidate_neighbours));
        }
        if (!all_forced_neighbours_are_feasible)
            return false;
        if (gp->min_deg == gp->max_deg)
            max_deg_incremented = true;
    }

    struct SearchData sd = {gp, have_short_path, gp_set_ptr, {min_degs[0], min_degs[1]},
            tentativeness_level, vertices_of_min_deg, vertices_of_min_deg_plus1};
    bool search_result = search(&sd, neighbours, candidate_neighbours, max_deg_incremented);
    if (tentativeness_level) {
        return search_result;
    } else {
        free_tree(&gp_set.tree_head);
        return true;
    }
}

void find_extremal_graphs(int n, int edge_count)
{
    if (global_low_splitting_level > 0 && global_split_number != 0 && n <= global_high_splitting_level)
        return;

    make_possible_graph_types(n, edge_count, MIN_GIRTH);

    graph g[MAXN];
    EMPTYGRAPH(g,1,MAXN);
    struct GraphPlus gp;
    make_graph_plus(g, 1, 0, 0, 0, &gp);
    visit_graph(&gp, 0, NULL);
}

int main(int argc, char *argv[])
{
    setlinebuf(stdout);

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
        global_low_splitting_level = atoi(argv[4]);
        global_high_splitting_level = atoi(argv[5]);
        global_split_number = atoi(argv[6]);
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

    printf("Canonicalisation calls: %lld\n", canonicalisation_calls);
    printf("Total graph count: %llu\n", global_graph_count);

    clean_up_graph_type_lists();
}
