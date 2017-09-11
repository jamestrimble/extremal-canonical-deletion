#include <stdlib.h>

void *ecalloc(size_t nmemb, size_t size);

void *emalloc(size_t n);

void print_array (char *name, int *arr, int min_n, int max_n);

void check_array (char *name, int *actual, int *expected, int min_n, int max_n);
