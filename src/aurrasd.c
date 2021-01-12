#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "structures.h"

#define REQUESTS_PIPE "./../tmp/requests_pipe"
#define STATUS_PIPE "./../tmp/status_pipe"

int flag = 1;
int n_filters = 0;
Filter* filter_array;

// Total Tasks
int total_tasks = 0;

// In Progress Tasks
int curr_in_progress = 0;
int max_in_progress = 10;
LinkedList* in_progress_list;

ssize_t readln(int fd, char* buf, size_t nbyte){
    // we assume that the buf is as big as nbytes
    int limite=0;
    char* read_buf = malloc(sizeof(char));
    while (read(fd,read_buf,1)>0 && limite < nbyte && (*(char*) read_buf)!='\n'){
        memcpy(&buf[limite], read_buf, 1);
        limite++;
    }
    return limite;
}

int filter_counter() {
    int lines = 0;
    int filter_file = open("./../etc/aurrasd.conf",O_RDONLY);
    if (filter_file == -1) printf("Something went wrong");
    else {
        char* buf = malloc(sizeof(char));
        while(read(filter_file, buf, 1) > 0) {
            if (!strcmp(buf,"\n")) lines++;
        }
    }
    return lines / 3;
}



void get_filters(int n_filters) {
    char* buf = malloc(256*sizeof(char));
    char* fname= malloc(sizeof(char));
    int filter_file = open("./../etc/aurrasd.conf",O_RDONLY);
    for (int it = 0; it < n_filters; it++) {
        memset(buf, 0, 256);
        readln(filter_file, buf, 256);
        strcpy(filter_array[it].name, buf);
        memset(buf, 0, 256);
        readln(filter_file, buf, 256);
        strcpy(filter_array[it].command, buf);
        memset(buf, 0, 256);
        readln(filter_file, buf, 256);
        filter_array[it].quantity=atoi(buf);
        memset(buf, 0, 256);
        filter_array[it].in_use = 0;
    }

    //for (int it = 0; it < n_filters; it++) printf("Name: %s, Command: %s, Quantity: %d\n", filter_array[it].name, filter_array[it].command, filter_array[it].quantity);

    free(buf);
    free(fname);
    close(filter_file);

}


int get_filter_index(char* arg){
    for(int i=0; i<n_filters; i++){
        if(!strcmp(filter_array[i].name,arg)) return i;
    }
    return -1;
}

int check_filter_availability(Request r) {
    // returns 0 if all filters are available, if not, returns different than 0
    int filters_available = 1;
    for(int it = 0; it < r.n_transformations; it++){
        // iterate transformations array
        for(int i=0; i < n_filters; i++) {
            // iterate available filters array
            if(!strcmp(filter_array[i].name, r.transformations[it])) {
                // found the desired transformation in the available filters array
                if (filter_array[i].in_use == filter_array[i].quantity) return -1;
            }
        }
    }

    return 0;
}


void releaseResources(LinkedList* list, int pid) {
    InProgress* curr = list->head;
    int curr_index = 0;

    printf("Gonna release the resources for the pid: %d\n", pid);

    if (curr == NULL) {
        printf("Release resources. List is NULL\n");
        return;
    }

    while(curr!=NULL) {
        // Iterating current task
        for(int it = 0; it < curr->n_transformations; it++) {
            if (curr->pid_array[it] == pid) {
                // found the task that the process is executing
                // so we need to release the resources in use
                printf("Found the pid! It belongs to the task nr: %d\n", curr->task_nr);
                int filter_index = get_filter_index(curr->task_array[it]);
                if (filter_index != -1) {
                    // resources are freed
                    filter_array[filter_index].in_use--;
                    printf("Released: 1 unit of the %s filter\n", filter_array[filter_index].name);
                    //kill(getppid(), SIGCHLD);
                }
                curr->done_transformations++;
                if (curr->done_transformations == curr->n_transformations) removeIndex(list, curr_index);
                return;
            }
        }
        curr = curr->next;
    }
}

void child_handler(int sig) {
    //function that handles SIGCHILD - when a child dies runs this
    pid_t child_pid = wait(NULL);
    printf("---Caught SIGCHILD! PID: %d---\n\n", child_pid);
    releaseResources(in_progress_list, child_pid);
}

void executor(int sleep_time) {
    printf("---PID %d, gonna sleep for %d seconds---\n\n", getpid(), sleep_time);
    sleep(sleep_time);
    printf("PID %d finishing executor now\n", getpid());
    exit(1);
}

void dispatch(Request r) {
    // allocate resources
    for (int it = 0; it < r.n_transformations; it++) {
         // iterate transformations array
        for(int i=0; i < n_filters; i++) {
            // iterate available filters array
            //printf("filter_array[%d]: %s, r.transformation[%d]: %s. Strcmp -> %d\n", i, filter_array[i].name,it, r.transformations[it], strcmp(filter_array[i].name, r.transformations[it]));
            if(!strcmp(filter_array[i].name, r.transformations[it])) {
                // found the desired transformation in the available filters array
                if (filter_array[i].in_use < filter_array[i].quantity){
                    filter_array[i].in_use++;
                    printf("A transformacao %s tem %d unidades em uso de %d totais.\n",filter_array[i].name,filter_array[i].in_use, filter_array[i].quantity);
                } 
            }
        }
    }

    // add task to in_progress array
    InProgress ip;
    ip.next = NULL;
    ip.task_nr = ++total_tasks;
    ip.done_transformations = 0;
    ip.n_transformations = r.n_transformations;
    strcpy(ip.dest_file,r.dest_file);
    for (int it = 0; it < r.n_transformations; it++) {
        strcpy(ip.task_array[it], r.transformations[it]);
        int child_pid = fork();
        if (!child_pid) {
            // child
            executor(it+2);
        }
        else {
            ip.pid_array[it] = child_pid;
        }
    }
    add(&ip, in_progress_list);
}

int main(int argc, char* argv[]) {
    // Variables initialization
    int server_pipes[3][2];

    int req_fifo = mkfifo(REQUESTS_PIPE, 0666);
    int requests_pipe = open(REQUESTS_PIPE, O_RDWR);

    mkfifo(STATUS_PIPE, 0666);
    int status_pipe = open(STATUS_PIPE, O_RDWR);

    if (requests_pipe == -1 || status_pipe == -1) return 0;

    int fd;
    char readbuf[80];
    char end[10];
    char s[256];
    int to_end;
    int read_bytes;
    int fconfig;

    // Server Setup
    // Signal Handlers
    signal(SIGCHLD, child_handler);

    // Filter Management
    n_filters = filter_counter() + 1;

    filter_array = malloc(n_filters * sizeof(Filter));
    in_progress_list = list_init();

    get_filters(n_filters);
    
    //printf("chars read: %ld \n",read(fd,&r,sizeof(r)));

    //printf("size: %d\n", sizeof(r));    
    
    while(flag) {
        printf("Before read\n");
        Request r;
        read(requests_pipe, &r, sizeof(Request));
        //printf("id_file: %s\nr.destfile: %s \nr.ntransformations: %d \n",r.id_file,r.dest_file,r.n_transformations);
        // Filter Verification - verification se em ou nao transformacoes
        
        int filters_exist = 1;
        for(int it = 0; it < r.n_transformations && filters_exist; it++){
            if (get_filter_index(r.transformations[it]) == -1) filters_exist = 0;

        }

        if (!filters_exist) {
            // One of the filters doesn't exist 
            // Todo
            printf("Filters don't exist\n");
        }
        else {
            // All of the filters exist
            // We now run check__filter_availability to see if all filters are available
            // Returns 0 if they are, -1 otherwise
            printf("Filters exist\n");
            int filters_available = check_filter_availability(r);
            while(filters_available) {
                // this will execute if some of the filters are not available
                //sigwait();
                
                sleep(5);
                filters_available = check_filter_availability(r);
                printf("---Filters availables: %d---\n\n",filters_available);
            }

            // Request is ready to be dispatched
            dispatch(r);
            InProgress* ip = getIndex(in_progress_list, 0);
            printf("GetIndex list: %d\n", ip->n_transformations);
        }

        printf("Finishing while loop\n");

    }

    close(requests_pipe);
    close(status_pipe);
    return 0;
}
