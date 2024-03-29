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
        retval += (unsigned long long) degs[w] * nb_deg_sum(g, w, degs);
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

