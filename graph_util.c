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

unsigned long long weighted_nb_nb_deg_sum(graph *g, int v, int *degs) {
    unsigned long long retval = 0;
    setword nb = g[v];
    while (nb) {
        int w;
        TAKEBIT(w, nb);
        retval += (unsigned long long) degs[w] << (nb_deg_sum(g, w, degs) & 31);
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

