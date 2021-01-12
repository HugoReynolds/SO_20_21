#include <stdlib.h>
#include <stdio.h>
#include "structures.h"

void main() {
    LinkedList* list = list_init();
    for (int it = 0; it < 10; it++) {
        printf("---Adding: %d---\n", it);
        InProgress ip;
        ip.n_transformations = it;
        add(&ip, list);
        printf("Added %d to position %d\n", it, it);
        InProgress* ipp = getIndex(list, it);
        printf("Index at position[%d] is %d\n\n", it, ipp->n_transformations);
    }

    removeIndex(list, 0);
    printf("---Getting: 5---\n");
    InProgress* ipp = getIndex(list, 5);
    printf("Index[%d]: %d\n\n", 5, ipp->n_transformations);
    printf("---Getting: 2---\n");
    ipp = getIndex(list, 2);
    printf("Index[%d]: %d\n\n", 2, ipp->n_transformations);
    for (int it = 0; it < list->tasks_in_progress; it++) {
        printf("Removing index %d. There are %d Tasks in Progress.\n", it, list->tasks_in_progress);
        removeIndex(list, it);
    }
    free_list(list);
}