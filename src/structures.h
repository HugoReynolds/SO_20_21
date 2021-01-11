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
    int n_transformations;
    char dest_file[256];
    char task_array[10][256];
    int pid_array[10];
    struct InProgress* next;
} InProgress;

typedef struct LinkedList {
    InProgress* head;
}

#endif