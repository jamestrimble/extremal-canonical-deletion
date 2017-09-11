#include <stdio.h>
#include "util.h"

// Adapted from "The Practice of Programming"
void *emalloc(size_t n)
{
    void *p;

    p = malloc(n);
    if (p == NULL) {
        fprintf(stderr, "calloc failed\n");
        exit(1);
    }
    return p;
}
void *ecalloc(size_t nmemb, size_t size)
{
    void *p;

    p = calloc(nmemb, size);
    if (p == NULL) {
        fprintf(stderr, "calloc failed\n");
        exit(1);
    }
    return p;
}


void print_array (char *name, int *arr, int min_n, int max_n)
{
    printf("%s ", name);
    if (min_n <= max_n)
        printf("%d", arr[min_n]);
    for (int n=min_n+1; n<=max_n; n++)
        printf(",%d", arr[n]);
    printf("\n");
}

void check_array (char *name, int *actual, int *expected, int min_n, int max_n)
{
    for (int n=min_n; n<=max_n; n++) {
        if (actual[n] != expected[n]) {
            printf("Unexpected value of %s[%d]: got %d, expected %d\n",
                    name, n, actual[n], expected[n]);
            exit(1);
        }
    }
}

