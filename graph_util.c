#define MAXN 64    /* Define this before including nauty.h */
#include "nauty.h"   /* which includes <stdio.h> and other system files */

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

int nb_nb_deg_sum(graph *g, int v, int *degs) {
    int deg_sum = 0;
    setword nb = g[v];
    while (nb) {
        int w;
        TAKEBIT(w, nb);
        deg_sum += nb_deg_sum(g, w, degs);
    }
    return deg_sum;
}

int num_neighbours_of_deg_d(graph *g, int v, int d, int *degs) {
    int deg_sum = 0;
    setword nb = g[v];
    while (nb) {
        int w;
        TAKEBIT(w, nb);
        deg_sum += degs[w]==d;
    }
    return deg_sum;
}

// Which pairs of vertices have a path of length max_path_len or less?
void all_pairs_check_for_short_path(graph *g, int n, int max_path_len, setword *have_short_path)
{
    for (int i=0; i<n; i++) {
        have_short_path[i] = g[i];
        ADDELEMENT(&have_short_path[i], i);
    }

    for (int k=2; k<=max_path_len; k++) {
        graph tmp[MAXN];
        for (int l=0; l<n; l++) {
            tmp[l] = have_short_path[l];
        }
        for (int i=0; i<n; i++) {
            for (int j=0; j<i; j++) {
                //  The next few lines do the same as the following, without an 'if'
                //  if (tmp[i] & g[j]) {
                //      ADDELEMENT(&have_short_path[i], j);
                //      ADDELEMENT(&have_short_path[j], i);
                //  }
                bool have_path = ((tmp[i] & g[j]) != 0ull);
                have_short_path[i] |= have_path * ((1ull<<63)>>j);
                have_short_path[j] |= have_path * ((1ull<<63)>>i);
            }
        }
    }
}

bool graph_min_deg_geq_target(graph *g, int n, int target, int *degs) {
    for (int i=0; i<n; i++)
        if (degs[i] < target)
            return false;
    return true;
}

bool graph_max_deg_eq_target(graph *g, int n, int target, int *degs) {
    int max_deg = 0;
    for (int i=0; i<n; i++)
        if (degs[i] > max_deg)
            max_deg = degs[i];
    return max_deg == target;
}

