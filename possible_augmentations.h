#include <stdbool.h>

struct Augmentation {
    struct Augmentation *next;   // Next in hash-set chain
    int num_vertices;
    int num_edges;
    int min_deg;
    int max_deg;
    int new_vertex_deg;
    bool max_deg_incremented;
};

bool augmentation_is_in_set(struct Augmentation *aug);

bool add_augmentation_to_set(struct Augmentation *aug);

void clean_up_augmentation_lists();
