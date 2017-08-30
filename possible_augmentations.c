#include "possible_augmentations.h"
#include "util.h"

#define HASH_TABLE_SIZE (1 << 20)

struct Augmentation *augmentation_list_heads[HASH_TABLE_SIZE] = {};

static unsigned int hash_augmentation(struct Augmentation *aug)
{
    unsigned int result = 17u;
    result = 31u*result + aug->num_vertices;
    result = 31u*result + aug->num_edges;
    result = 31u*result + aug->min_deg;
    result = 31u*result + aug->max_deg;
    result = 31u*result + aug->new_vertex_deg;
    result = 31u*result + aug->max_deg_incremented;
    return result % HASH_TABLE_SIZE;
}

static bool augmentations_equal(struct Augmentation *aug0, struct Augmentation *aug1)
{
    return aug0->num_vertices == aug1->num_vertices &&
           aug0->num_edges == aug1->num_edges &&
           aug0->min_deg == aug1->min_deg &&
           aug0->max_deg == aug1->max_deg &&
           aug0->new_vertex_deg == aug1->new_vertex_deg &&
           aug0->max_deg_incremented == aug1->max_deg_incremented;
}

bool augmentation_is_in_set(struct Augmentation *aug)
{
    struct Augmentation *head = augmentation_list_heads[hash_augmentation(aug)];
    while (head) {
        if (augmentations_equal(head, aug)) {
            return true;
        }
        head = head->next;
    }
    return false;
}

// Returns true if the augmentation was added, and false if it was already present
bool add_augmentation_to_set(struct Augmentation *aug)
{
    if (!augmentation_is_in_set(aug)) {
        unsigned int hash_val = hash_augmentation(aug);
        struct Augmentation *aug_copy = emalloc(sizeof(*aug_copy));
        *aug_copy = *aug;
        aug_copy->next = augmentation_list_heads[hash_val];
        augmentation_list_heads[hash_val] = aug_copy;
        return true;
    } else {
        return false;
    }
}

void clean_up_augmentation_lists()
{
    for (int i=0; i<HASH_TABLE_SIZE; i++) {
        struct Augmentation *head = augmentation_list_heads[i];
        while (head) {
            struct Augmentation *next = head->next;
            free(head);
            head = next;
        }
    }
}
