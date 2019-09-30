#include "graph_plus.h"
#include "possible_graph_types.h"
#include "util.h"

#define HASH_TABLE_SIZE (1 << 20)

#define MAX(a,b) ((a)>(b) ? (a) : (b))

static const int MAX_RECORDED_EXTREMAL_6 = 40;
static const int EXTREMAL_6[] = {0,0,1,2,3,4,6,7,9,10,12,14,16,18,21,22,24,26,29,31,
        34,36,39,42,45,48,52,53,56,58,61,64,67,70,74,77,81,84,88,92,96};
static const int MAX_RECORDED_EXTREMAL_5 = 32;
static const int EXTREMAL_5[] = {0,1,2,3,5,6,8,10,12,15,16,18,21,23,26,28,31,34,38,
        41,44,47,50,54,57,61,65,68,72,76,80,85};
struct GraphType *graph_type_list_heads[HASH_TABLE_SIZE] = {};

static unsigned int hash_graph_type(struct GraphType *graph_type)
{
    unsigned int result = 17u;
    result = 31u*result + graph_type->num_vertices;
    result = 31u*result + graph_type->num_edges_minus_min_deg;
    result = 31u*result + graph_type->max_deg;
    return result % HASH_TABLE_SIZE;
}

static bool graph_types_equal(struct GraphType *graph_type0, struct GraphType *graph_type1)
{
    return graph_type0->num_vertices == graph_type1->num_vertices &&
           graph_type0->num_edges_minus_min_deg == graph_type1->num_edges_minus_min_deg &&
           graph_type0->max_deg == graph_type1->max_deg;
}

static bool ok_not_to_try_min_deg(int n, int min_deg, int edge_count)
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

static bool girth_at_least_6_can_place_enough_edges_to_star(int n, int min_deg, int max_deg,
        int edge_count)
{
    // Taking advantage of the fact that if min girth is 6, then
    // there can be no extra edges added among vertices in the big star
    int tree_order = min_deg*max_deg + 1;
    int tree_edge_count = tree_order-1;
    int extra_edges_required = edge_count - tree_edge_count;
    int num_non_tree_vertices = n - tree_order;
    // i is number of vertices of degree max_deg in the entire graph
    int top = min_deg==max_deg ? n : n-1;
    for (int i=1; i<=top; i++) {
        if (i*max_deg + (n-i)*min_deg <= edge_count*2) {
            // b is an upper bound on the number of non-tree vertices with degree max_deg
            int b = i - 1;
            if (b > num_non_tree_vertices)
                b = num_non_tree_vertices;
            if (b*max_deg + (num_non_tree_vertices-b)*(max_deg-1) >= extra_edges_required)
                return true;
        }
    }
    return false;
}

static bool min_and_max_deg_are_feasible(int n, int min_deg, int max_deg, int edge_count, int min_girth)
{
    if (max_deg == 0)
        return min_deg==0 && edge_count==0;

    if (max_deg >= n)
        return false;

    if (min_deg*max_deg + 1 > n)
        return false;

    if (min_deg + (max_deg*(n-1)) < edge_count*2)
        return false;

    if (max_deg + (min_deg*(n-1)) > edge_count*2)
        return false;

    if (min_girth >= 6 && !girth_at_least_6_can_place_enough_edges_to_star(
                n, min_deg, max_deg, edge_count))
        return false;

    if (min_girth == 6 && n <= MAX_RECORDED_EXTREMAL_6 && edge_count > EXTREMAL_6[n])
        return false;

    if (min_girth == 5 && n <= MAX_RECORDED_EXTREMAL_5 && edge_count > EXTREMAL_5[n])
        return false;

    return true;
}

static void make_possible_graph_types_recurse(int n, int edge_count, int min_deg, int max_deg, int min_girth)
{
    if (n == 1)
        return;

    struct GraphType graph_type = {
                .num_vertices=n,
                .num_edges_minus_min_deg=edge_count-min_deg,
                .max_deg=max_deg
            };

    if (add_graph_type_to_set(&graph_type, min_deg)) {
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
                    if (min_and_max_deg_are_feasible(n-1, i, j, small_g_ec, min_girth)) {
                        make_possible_graph_types_recurse(n-1, small_g_ec, i, j, min_girth);
                    }
                }
            }
        }
    }
}

void make_possible_graph_types(int n, int edge_count, int min_girth)
{
    for (int min_deg=0; min_deg<=MIN_DEG_UPPER_BOUND; min_deg++) {
        for (int max_deg=min_deg; max_deg<=MAX_DEG_UPPER_BOUND; max_deg++) {
            if (min_and_max_deg_are_feasible(n, min_deg, max_deg, edge_count, min_girth)) {
                make_possible_graph_types_recurse(n, edge_count, min_deg, max_deg, min_girth);
            }
        }
    }
}

struct GraphType * find_graph_type_in_set(struct GraphType *graph_type)
{
    struct GraphType *head = graph_type_list_heads[hash_graph_type(graph_type)];
    while (head) {
        if (graph_types_equal(head, graph_type)) {
            return head;
        }
        head = head->next;
    }
    return NULL;
}

// Returns true if the graph_type was added, and false if it was already present
bool add_graph_type_to_set(struct GraphType *graph_type, int min_deg)
{
    struct GraphType *gt = find_graph_type_in_set(graph_type);
    if (!gt) {
        unsigned int hash_val = hash_graph_type(graph_type);
        struct GraphType *graph_type_copy = emalloc(sizeof(*graph_type_copy));
        *graph_type_copy = *graph_type;
        graph_type_copy->min_degs = 0;
        ADDELEMENT(&graph_type_copy->min_degs, min_deg);
        graph_type_copy->next = graph_type_list_heads[hash_val];
        graph_type_list_heads[hash_val] = graph_type_copy;
//        printf("TYPE %d %d %d %d\n",
//                graph_type->num_vertices,
//                graph_type->num_edges,
//                graph_type->min_deg,
//                graph_type->max_deg);
        return true;
    } else {
        if (!ISELEMENT(&gt->min_degs, min_deg)) {
            ADDELEMENT(&gt->min_degs, min_deg);
            return true;
        } else {
            return false;
        }
    }
}

void clean_up_graph_type_lists()
{
    for (int i=0; i<HASH_TABLE_SIZE; i++) {
        struct GraphType *head = graph_type_list_heads[i];
        while (head) {
            struct GraphType *next = head->next;
            free(head);
            head = next;
        }
    }
}
