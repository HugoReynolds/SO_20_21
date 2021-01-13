#ifndef structures_h
#define structures_h

typedef struct Request {
    int n_transformations;
    char id_file[256];
    char dest_file[256];
    char transformations[10][256];
} Request;

typedef struct Queued_Request{
    char id_file[256];
    char dest_file[256];
    char pipe_name[128];
    int child_pipes[10][2];
} Queued_Request;

typedef struct Filter{
    char name[256];
    char command[256];
    int quantity;
    int in_use;
} Filter;

typedef struct InProgress {
    int task_nr;
    int done_transformations;
    int n_transformations;
    char origin_file[256];
    char dest_file[256];
    char task_array[10][256];
    int pid_array[10];
    int pipe_matrix[10][2];
    struct InProgress* next;
} InProgress;

typedef struct LinkedList{
    struct InProgress* head;
    int tasks_in_progress;
} LinkedList;

LinkedList* list_init();
void add(InProgress* data, LinkedList* list);
InProgress* getIndex(LinkedList* list, int index);
void removeIndex(LinkedList* list, int index);
void free_list(LinkedList* list);

#endif
