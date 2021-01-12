#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "structures.h"

LinkedList* list_init() {
    LinkedList* list = malloc(sizeof(LinkedList));
    if (!list) {
        return NULL;
    }
    list->head = NULL;
    list->tasks_in_progress = 0;
    return list;
}

void add(InProgress* data, LinkedList* list) {
    InProgress* curr = NULL;

    if (list->head == NULL) {
        list->head = malloc(sizeof(InProgress));
        list->head->task_nr = data->task_nr;
        list->head->done_transformations = 0;
        list->head->n_transformations = data->n_transformations;
        strcpy(list->head->dest_file, data->dest_file);
        for (int it = 0; it < data->n_transformations; it++) {
            strcpy(list->head->task_array[it], data->task_array[it]);
            list->head->pid_array[it] = data->pid_array[it];
        }
        list->head->next = NULL;
        list->tasks_in_progress++;
    }
    else {
        curr = list->head;
        while(curr->next != NULL) curr = curr->next;
        curr->next= malloc(sizeof(InProgress));
        curr = curr->next;
        curr->task_nr = data->task_nr;
        curr->done_transformations = 0;
        curr->n_transformations = data->n_transformations;
        strcpy(curr->dest_file, data->dest_file);
        for (int it = 0; it < data->n_transformations; it++) {
            strcpy(curr->task_array[it], data->task_array[it]);
            curr->pid_array[it] = data->pid_array[it];
        }
        curr->next = NULL;
        list->tasks_in_progress++;
    }
}

InProgress* getIndex(LinkedList* list, int index) {
    if (index >= list->tasks_in_progress) {
        printf("Index is too large\n");
        return NULL;
    }
    int it = 0;
    InProgress* curr = NULL;
    curr = list->head;
    while (it < index && curr->next!=NULL) {
        //printf("Curr[%d]->NTR: %d\n", it, curr->n_transformations);
        curr = curr->next;
        it++;
    }
    //printf("Curr->n_trans: %d\n", curr->n_transformations);
    return curr;
}

void removeIndex(LinkedList* list, int index) {
    if (index >= list->tasks_in_progress) {
        printf("No such item with that index\n");
        return;
    }
    
    InProgress* temp;
    InProgress* prev;
    temp = list->head;
    
    // check if list is null

    if (temp == NULL) {
        return;
    }

    // remove index 0
    if (index == 0) {
        prev = temp;
        temp = temp->next;
        free(prev);
        // reconnect the the head of the list...
        list->head = temp;
        list->tasks_in_progress--;
        return;
    }

    // remove index 1+
    int it = 0;
    
    prev = temp;
    while (it < index && temp!=NULL) {
        prev = temp;
        temp = temp->next;
        it++;
    }

    if (temp == NULL) return;

    prev->next = temp->next;
    list->tasks_in_progress--;

    free(temp);

}

void free_list(LinkedList* list) {
    InProgress* tmpPtr = list->head;
    InProgress* followPtr;
    while (tmpPtr != NULL) {
        followPtr = tmpPtr;
        tmpPtr = tmpPtr->next;
        free(followPtr);
    }
    free(list);
}
