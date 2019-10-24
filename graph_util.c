#include "graph_plus.h"
#include "graph_util.h"

#include <stdbool.h>

int nb_deg_sum(graph *g, int v, int *degs) {
    int deg_sum = 0;
    setword nb = g[v];
    while (nb) {
        int w;
        TAKEBIT(w, nb);
        deg_sum += degs[w];
    }
    return deg_sum;
}

int nb_deg_sum_self_contained(graph *g, int v) {
    int deg_sum = 0;
    setword nb = g[v];
    while (nb) {
        int w;
        TAKEBIT(w, nb);
        deg_sum += POPCOUNT(g[w]);
    }
    return deg_sum;
}

unsigned long long weighted_nb_nb_deg_sum(graph *g, int v) {
    unsigned long long retval = 0;
    setword nb = g[v];
    while (nb) {
        int w;
        TAKEBIT(w, nb);
        retval += (unsigned long long) POPCOUNT(g[w]) << (nb_deg_sum_self_contained(g, w) & 31);
    }
    return retval;
}

unsigned long long fast_weighted_nb_nb_deg_sum(graph *g, int v,
        unsigned long long *vtx_score) {
    unsigned long long retval = 0;
    setword nb = g[v];
    while (nb) {
        int w;
        TAKEBIT(w, nb);
        retval += vtx_score[w];
    }
    return retval;
}

int num_neighbours_of_deg_d(graph *g, int v, int d, int *degs) {
    int count = 0;
    setword nb = g[v];
    while (nb) {
        int w;
        TAKEBIT(w, nb);
        count += degs[w]==d;
    }
    return count;
}

// Which pairs of vertices have a path of length max_path_len or less?
void all_pairs_check_for_short_path(graph *g, int n, int max_path_len, setword *have_short_path)
{
    for (int i=0; i<n; i++) {
        have_short_path[i] = g[i] | bit[i];
    }

    for (int k=2; k<=max_path_len; k++) {
        for (int i=1; i<n; i++) {
            graph have_short_path_i = have_short_path[i];   // micro-optimisation
            graph tmp = have_short_path_i;
            for (int j=0; j<i; j++) {
                //  The next few lines do the same as the following, without an 'if'
                //  if (tmp & g[j]) {
                //      ADDELEMENT(&have_short_path_i, j);
                //      ADDELEMENT(&have_short_path[j], i);
                //  }
                setword have_path = ((tmp & g[j]) != 0ull) ? ~0ull : 0ull;
                have_short_path_i |= have_path & bit[j];
                have_short_path[j] |= have_path & bit[i];
            }
            have_short_path[i] = have_short_path_i;
        }
    }
}

void extend_short_path_arr(graph *g, int n, int max_path_len, setword *have_short_path,
        setword *parent_have_short_path)
{
    if (max_path_len == 2) {
        for (int i=0; i<n-1; i++)
            have_short_path[i] = parent_have_short_path[i];
        setword last_vtx_1_path = g[n-1];
        setword last_vtx_2_path = 0;
        setword tmp = last_vtx_1_path;
        while (tmp) {
            int v;
            TAKEBIT(v, tmp);
            last_vtx_2_path |= g[v];
        }
        last_vtx_2_path &= ~bit[n-1];
        tmp = last_vtx_1_path;
        while (tmp) {
            int v;
            TAKEBIT(v, tmp);
            have_short_path[v] |= last_vtx_1_path;
        }
        have_short_path[n-1] = last_vtx_1_path | last_vtx_2_path | bit[n-1];
        tmp = have_short_path[n-1];
        while (tmp) {
            int v;
            TAKEBIT(v, tmp);
            have_short_path[v] |= bit[n-1];
        }
    } else if (max_path_len == 3) {
        for (int i=0; i<n-1; i++)
            have_short_path[i] = parent_have_short_path[i];
        setword last_vtx_1_path = g[n-1];
        setword last_vtx_2_path = 0;
        setword last_vtx_3_path = 0;
        setword tmp = last_vtx_1_path;
        while (tmp) {
            int v;
            TAKEBIT(v, tmp);
            last_vtx_2_path |= g[v];
        }
        last_vtx_2_path &= ~bit[n-1];
        tmp = last_vtx_2_path;
        while (tmp) {
            int v;
            TAKEBIT(v, tmp);
            last_vtx_3_path |= g[v];
            have_short_path[v] |= last_vtx_1_path;
        }
        last_vtx_3_path &= ~bit[n-1];
        tmp = last_vtx_1_path;
        while (tmp) {
            int v;
            TAKEBIT(v, tmp);
            have_short_path[v] |= (last_vtx_1_path | last_vtx_2_path);
        }
        have_short_path[n-1] = last_vtx_1_path | last_vtx_2_path | last_vtx_3_path | bit[n-1];
        tmp = have_short_path[n-1];
        while (tmp) {
            int v;
            TAKEBIT(v, tmp);
            have_short_path[v] |= bit[n-1];
        }
    } else {
        all_pairs_check_for_short_path(g, n, max_path_len, have_short_path);
    }
}

void show_graph(struct GraphPlus *gp)
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

////////////////////////////////////////////////////////////////////////////////
//  Canonicalisation
////////////////////////////////////////////////////////////////////////////////

#define INSERTION_SORT(type, arr, arr_len, swap_condition) do { \
    for (int i=1; i<arr_len; i++) {                             \
        for (int j=i; j>=1; j--) {                              \
            if (swap_condition) {                               \
                type tmp = arr[j-1];                            \
                arr[j-1] = arr[j];                              \
                arr[j] = tmp;                                   \
            } else {                                            \
                break;                                          \
            }                                                   \
        }                                                       \
    }                                                           \
} while(0);

// These are probably not actually orbits!!!
static int vtx_to_orbit[MAXN];
static setword orbits[MAXN];
static int incumbent_order[MAXN];

void possibly_update_incumbent(graph *g, int n, int *order, int order_len,
        setword *vv_set, int num_sets, graph *incumbent_g)
{
    for (int i=0; i<num_sets; i++)
        order[order_len++] = FIRSTBITNZ(vv_set[i]);

    int order_inv[MAXN];
    for (int i=0; i<n; i++)
        order_inv[order[i]] = i;

    graph new_g[MAXN] = {};
    for (int i=0; i<n; i++) {
        setword row = g[order[i]];
        while (row) {
            int w;
            TAKEBIT(w, row);
            ADDELEMENT(&new_g[i], order_inv[w]);
        }
    }

    for (int i=0; i<n; i++) {
        if (new_g[i] < incumbent_g[i]) {
            for (int j=0; j<n; j++)
                incumbent_g[j] = new_g[j];
            for (int j=0; j<n; j++)
                incumbent_order[j] = order[j];
            return;
        } else if (new_g[i] > incumbent_g[i]) {
            return;
        }
    }

    // The graph is the same as the incumbent.  Update orbits.
    for (int i=0; i<n; i++) {
        int v = order[i];
        int w = incumbent_order[i];
        int orb_v = vtx_to_orbit[v];
        int orb_w = vtx_to_orbit[w];
        if (orb_v != orb_w) {
            // combine orbits of v and w
            setword s = orbits[orb_w];
            while (s) {
                int u;
                TAKEBIT(u, s);
                vtx_to_orbit[u] = orb_v;
            }
            orbits[orb_v] |= orbits[orb_w];
        }
    }
}

bool only_singleton_sets_exist(setword *vv_set, int num_sets)
{
    for (int i=0; i<num_sets; i++)
        if (POPCOUNT(vv_set[i]) > 1)
            return false;
    return true;
}

int choose_set_for_splitting(setword *vv_set, int num_sets)
{
    int min_set_len = 99999;
    int best_set_idx = -1;
    for (int i=0; i<num_sets; i++) {
        int len = POPCOUNT(vv_set[i]);
        if (len < min_set_len) {
            min_set_len = len;
            best_set_idx = i;
            if (len == 1)
                break;    // just to save time
        }
    }
    return best_set_idx;
}

void canon_search(graph *g, graph *incumbent_g, int n,
        setword *vv_set, int num_sets, int *order, int order_len)
{
    //symmetry breaking
    if (incumbent_g[0] != ~0ull) {   // only if incumbent has been set
        for (int i=0; i<order_len; i++) {
            int v = order[i];
            int incumbent_v = incumbent_order[i];
            if (v != incumbent_v) {
                if (ISELEMENT(&orbits[vtx_to_orbit[v]], incumbent_v)) {
                    return;
                } else {
                    break;
                }
            }
            if (vtx_to_orbit[v] != v || POPCOUNT(orbits[v]) != 1) {
                break;
            }
        }
    }

    if (only_singleton_sets_exist(vv_set, num_sets)) {
        possibly_update_incumbent(g, n, order, order_len, vv_set, num_sets, incumbent_g);
        return;
    }

    int best_set_idx = choose_set_for_splitting(vv_set, num_sets);

    setword vv = vv_set[best_set_idx];
    while (vv) {
        int w;
        TAKEBIT(w, vv);
        vv_set[best_set_idx] ^= bit[w];   // temporarily remove w
        setword new_vv_set[MAXN];
        int new_num_sets = 0;
        for (int j=0; j<num_sets; j++) {
            // McSplit-style splitting
            setword a = vv_set[j] & g[w];
            setword b = vv_set[j] & ~g[w];
            if (a != 0)
                new_vv_set[new_num_sets++] = a;
            if (b != 0)
                new_vv_set[new_num_sets++] = b;
        }
        order[order_len] = w;
        canon_search(g, incumbent_g, n, new_vv_set, new_num_sets, order, order_len+1);
        vv_set[best_set_idx] ^= bit[w];   // add w back
    }
}

int make_vv_sets(graph *g, int n, setword *vv_set)
{
    int degs[MAXN];
    setword vv_by_deg[MAXN] = {};
    setword degs_used = 0;
    for (int i=0; i<n; i++) {
        int deg = POPCOUNT(g[i]);
        degs[i] = deg;
        vv_by_deg[deg] |= bit[i];
        degs_used |= bit[deg];
    }

    unsigned long long vtx_score[MAXN];
    for (int i=0; i<n; i++)
        vtx_score[i] = (unsigned long long) degs[i] << (nb_deg_sum(g, i, degs) & 31);

    unsigned long long vtx_nnds[MAXN];
    for (int i=0; i<n; i++)
        vtx_nnds[i] = fast_weighted_nb_nb_deg_sum(g, i, vtx_score);

    // sort by (degree, weighted_nb_nb_deg_sum)
    int vv[MAXN];
    int j = 0;
    int current_set_num = -1;
    while (degs_used) {
        int deg;
        TAKEBIT(deg, degs_used);
        setword tmp = vv_by_deg[deg];
        int old_j = j;
        while (tmp) {
            int v;
            TAKEBIT(v, tmp);
            vv[j++] = v;
        }
        int *ww = vv + old_j;
        int ww_len = j - old_j;
        INSERTION_SORT(int, ww, ww_len, vtx_nnds[ww[j]] < vtx_nnds[ww[j-1]]);

        ++current_set_num;
        vv_set[current_set_num] = bit[ww[0]];
        unsigned long long prev_nnds = vtx_nnds[ww[0]];
        for (int i=1; i<ww_len; i++) {
            int v = ww[i];
            if (vtx_nnds[v] != prev_nnds) {
                ++current_set_num;
                vv_set[current_set_num] = 0;
            }
            vv_set[current_set_num] |= bit[v];
            prev_nnds = vtx_nnds[v];
        }
    }

    return current_set_num + 1;
}

void make_canonical(graph *g, int n, graph *canon_g)
{
#ifdef SELF_CONTAINED
    for (int i=0; i<n; i++) {
        vtx_to_orbit[i] = i;
        orbits[i] = bit[i];
    }

    setword vv_set[MAXN];
    int num_sets = make_vv_sets(g, n, vv_set);

    graph incumbent_g[MAXN] = {};
    for (int i=0; i<n; i++)
        incumbent_g[i] = ~0ull;
    int order[MAXN];
    canon_search(g, incumbent_g, n, vv_set, num_sets, order, 0);

    for (int i=0; i<n; i++)
        canon_g[i] = incumbent_g[i];

#else
    static DEFAULTOPTIONS_GRAPH(options);
    options.getcanon = TRUE;
    options.tc_level = 0;
    static statsblk stats;
    int lab[MAXN],ptn[MAXN],orbits[MAXN];
    EMPTYGRAPH(canon_g,1,MAXN);
    setword workspace[120];
    nauty(g,lab,ptn,NULL,orbits,&options,&stats,workspace,120,1,n,canon_g);
#endif
}

