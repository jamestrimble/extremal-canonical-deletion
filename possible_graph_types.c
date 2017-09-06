#include "possible_graph_types.h"
#include "util.h"

#define HASH_TABLE_SIZE (1 << 20)

struct GraphType *graph_type_list_heads[HASH_TABLE_SIZE] = {};

static unsigned int hash_graph_type(struct GraphType *graph_type)
{
    unsigned int result = 17u;
    result = 31u*result + graph_type->num_vertices;
    result = 31u*result + graph_type->num_edges;
    result = 31u*result + graph_type->min_deg;
    result = 31u*result + graph_type->max_deg;
    return result % HASH_TABLE_SIZE;
}

static bool graph_types_equal(struct GraphType *graph_type0, struct GraphType *graph_type1)
{
    return graph_type0->num_vertices == graph_type1->num_vertices &&
           graph_type0->num_edges == graph_type1->num_edges &&
           graph_type0->min_deg == graph_type1->min_deg &&
           graph_type0->max_deg == graph_type1->max_deg;
}

bool graph_type_is_in_set(struct GraphType *graph_type)
{
    struct GraphType *head = graph_type_list_heads[hash_graph_type(graph_type)];
    while (head) {
        if (graph_types_equal(head, graph_type)) {
            return true;
        }
        head = head->next;
    }
    return false;
}

// Returns true if the graph_type was added, and false if it was already present
bool add_graph_type_to_set(struct GraphType *graph_type)
{
    if (!graph_type_is_in_set(graph_type)) {
        unsigned int hash_val = hash_graph_type(graph_type);
        struct GraphType *graph_type_copy = emalloc(sizeof(*graph_type_copy));
        *graph_type_copy = *graph_type;
        graph_type_copy->next = graph_type_list_heads[hash_val];
        graph_type_list_heads[hash_val] = graph_type_copy;
        return true;
    } else {
        return false;
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
