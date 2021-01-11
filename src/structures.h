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

#endif