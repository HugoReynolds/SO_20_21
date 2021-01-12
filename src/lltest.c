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
        InProgress* ipp = getIndex(list, it);
        printf("Ipp Transfs: %d\n\n", ipp->n_transformations);
    }

    printf("---Getting: 5---\n");
    InProgress* ipp = getIndex(list, 5);
    printf("Index[%d]: %d\n\n", 5, ipp->n_transformations);
    printf("---Removing: 1---\n");
    removeIndex(list, 1);
    printf("\n\n");
    printf("---Getting: 2---\n");
    ipp = getIndex(list, 2);
    printf("Index[%d]: %d\n\n", 2, ipp->n_transformations);

    for (int it = 0; it < list->tasks_in_progress; it++) {
        removeIndex(list, it);
    }

    free_list(list);
}