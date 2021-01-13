#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
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

// Strings with locations
char* conf_dir;
char* trans_dir;

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

char *my_itoa(int num, char *str)
{
        if(str == NULL)
        {
                return NULL;
        }
        sprintf(str, "%d", num);
        return str;
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

    printf("--- Available Filters ---\n\n");
    for (int it = 0; it < n_filters; it++) printf("Filter nr: %d. Name: %s, Quantity: %d\n", it, filter_array[it].name, filter_array[it].quantity);

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

    //printf("---Gonna release the resources for the pid: %d---\n\n", pid);

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
                int filter_index = get_filter_index(curr->task_array[it]);
                if (filter_index != -1) {
                    // resources are freed
                    filter_array[filter_index].in_use--;
                    printf("A: [%d], B: [%d], E: [%d], R: [%d], L: [%d]\n\n", filter_array[0].in_use,filter_array[1].in_use,filter_array[2].in_use,filter_array[3].in_use,filter_array[4].in_use);
                }
                curr->done_transformations++;
                printf("--- Task Nr: %d. %d/%d transformations done.--- \n\n", curr->task_nr, curr->done_transformations, curr->n_transformations);
                if (curr->done_transformations == curr->n_transformations) {
                    printf("\n --- Task [%d] is finished! --- \n\n", curr->task_nr);
                    removeIndex(list, curr_index);
                }
                return;
            }
        }
        curr = curr->next;
        curr_index++;
    }
}

void signal_handler(int sig) {
    //function that handles signals
    pid_t pid = wait(NULL);
    switch (sig){
        case SIGCHLD:
            printf("Received SIGCHILD with Pid: %d!\n\n",pid);
            releaseResources(in_progress_list, pid);
            break;
        case SIGUSR1:
            printf("Received SIGUSR1 with Pid: %d!\n\n",pid);
            break;
        default:
            printf("Some other signal was received\n\n");
            break;
    }
}

void executor(char* trans) {
    execl(trans, trans, NULL);
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
                    printf("A: [%d], B: [%d], E: [%d], R: [%d], L: [%d]\n\n", filter_array[0].in_use,filter_array[1].in_use,filter_array[2].in_use,filter_array[3].in_use,filter_array[4].in_use);
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
    strcpy(ip.origin_file,r.id_file);
    strcpy(ip.dest_file,r.dest_file);
    for (int it = 0; it < r.n_transformations; it++) {
        strcpy(ip.task_array[it], r.transformations[it]);
        // format task string with dir
        int filter_index = get_filter_index(ip.task_array[it]);

        char* task_with_dir = malloc((strlen(filter_array[filter_index].command) + strlen(trans_dir) + 1) * sizeof(char));
        memcpy(task_with_dir, trans_dir, strlen(trans_dir));
        memcpy(task_with_dir + strlen(trans_dir), filter_array[filter_index].command, strlen(filter_array[filter_index].command));
        memcpy(task_with_dir + strlen(trans_dir) + strlen(filter_array[filter_index].command), "\0", 1);
        
        if (r.n_transformations == 1) {
            // if it's only one transformation
            // child opens both files for stdin and stdout directly

            int open_file = open(ip.origin_file, O_RDONLY);
            dup2(open_file, 0);
            close(open_file);

            int dest_file = open(ip.dest_file, O_WRONLY | O_APPEND | O_CREAT, 0644);
            dup2(dest_file, 1);
            close(dest_file);

            execl(task_with_dir, task_with_dir, NULL);

        }
        else {
            // pipes
            if (it > 1) {
                close(ip.pipe_matrix[it-2][0]);
                close(ip.pipe_matrix[it-2][1]);
            }

            if (it < (r.n_transformations - 1)) {
                // not the last iteration
                pipe(ip.pipe_matrix[it]);
            }

            // fork

            int ip.pid_array[it] = fork();
            if (!ip.pid_array[it]) {
                // child
                if (!it) {
                    printf("First child!\n");
                    // first child
                    // closing reading end
                    close(ip.pipe_matrix[it][0]);
                    printf("Origin file: %s\n\n", ip.origin_file);
                    int origin_file = open(ip.origin_file, O_RDONLY);
                    dup2(origin_file, 0);
                    close(origin_file);

                    // redirect writing end
                    dup2(ip.pipe_matrix[it][1],1);
                    close(ip.pipe_matrix[it][1]);

                    execl(task_with_dir, task_with_dir, NULL);
                }
                else{ 
                    if (it == ip.n_transformations-1) {
                        printf("Last child!\n");
                        // last child
                        // close writing end
                        printf("Destination file: %s\n\n", ip.dest_file);
                        char dest_fname[256];
                        strcat(dest_fname, ip.dest_file);
                        time_t rawtime;
                        struct tm * timeinfo;
                        time(&rawtime);
                        timeinfo = localtime(&rawtime);
                        char secs[256];
                        my_itoa(timeinfo->tm_sec, secs);
                        char mins[256];
                        my_itoa(timeinfo->tm_min, mins);
                        char hs[256];
                        my_itoa(timeinfo->tm_hour, hs);
                        strcat(dest_fname, hs);
                        strcat(dest_fname, ":");
                        strcat(dest_fname, mins);
                        strcat(dest_fname, ":");
                        strcat(dest_fname, secs);
                        strcat(dest_fname, ".txt");
                        printf("Dest_fname: %s\n", dest_fname);
                        close(ip.pipe_matrix[it-1][1]);
                        int dest_file = open(dest_fname, O_WRONLY | O_APPEND | O_CREAT);
                        dup2(dest_file, 1);
                        close(dest_file);

                        // redirect reading end
                        dup2(ip.pipe_matrix[it-1][0],0);
                        close(ip.pipe_matrix[it-1][0]);

                        execl(task_with_dir, task_with_dir, NULL);
                        //execl("/bin/bash", "/bin/bash", "-c", "echo \"hello world!\"",  NULL);
                        exit(1);
                    }
                    else {
                        printf("One of the middle child!\n");
                        // one of the middle childs
                        // close reading end
                        close(ip.pipe_matrix[it][0]);

                        // close writing end
                        close(ip.pipe_matrix[it-1][1]);

                        // redirect reading end
                        dup2(ip.pipe_matrix[it-1][0],0);
                        close(ip.pipe_matrix[it-1][0]);

                        //redirect writing end
                        dup2(ip.pipe_matrix[it][1],1);
                        close(ip.pipe_matrix[it][1]);

                        execl(task_with_dir, task_with_dir, NULL);
                    }
                }
            }
            else {
                // Father
            }
        }
    }

    if (r.n_transformations > 1) {
        close(ip.pipe_matrix[ip.n_transformations-2][0]);
        close(ip.pipe_matrix[ip.n_transformations-2][1]);
    }

    add(&ip, in_progress_list);

    printf("\n+++ Added Task nr %d to InProgess +++\n\n", ip.task_nr);
    for(int it = 0; it < ip.n_transformations; it++) printf("PID: %d -> Filter %s\n", ip.pid_array[it], ip.task_array[it]);
}

int main(int argc, char* argv[]) {
    if (argc != 3) return 1;
    else {
        conf_dir = strdup(argv[1]);
        trans_dir = strdup(argv[2]);
        printf("Conf: %s, Trans_dir: %s\n", conf_dir, trans_dir);
    }
    // Variables initialization
    int server_pipes[3][2];

    int req_fifo = mkfifo(REQUESTS_PIPE, 0666);
    int requests_pipe = open(REQUESTS_PIPE, O_RDWR);

    mkfifo(STATUS_PIPE, 0666);
    int status_pipe = open(STATUS_PIPE, O_RDWR);

    if (requests_pipe == -1 || status_pipe == -1) return 0;

    printf("\n---PID Main: %d---\n\n", getpid());

    int fd;
    char readbuf[80];
    char end[10];
    char s[256];
    int to_end;
    int read_bytes;
    int fconfig;

    // Server Setup
    // Signal Handlers
    signal(SIGCHLD, signal_handler);
    signal(SIGUSR1, signal_handler);

    // Filter Management
    n_filters = filter_counter() + 1;

    filter_array = malloc(n_filters * sizeof(Filter));
    in_progress_list = list_init();

    get_filters(n_filters);
    
    //printf("chars read: %ld \n",read(fd,&r,sizeof(r)));

    //printf("size: %d\n", sizeof(r));    
    
    while(flag) {
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
            int filters_available = check_filter_availability(r);
            while(filters_available) {
                // this will execute if some of the filters are not available
                //printf("/// Inside Filters are not Available while \\\\\n\n");
                /*
                sigset_t wset;
                sigemptyset(&wset);
                sigaddset(&wset, SIGUSR1);
                int sig;
                sigwait(&wset, &sig);
                */
                sleep(5);
                //printf("---Before CheckFilterAvailability---\n\n");
                filters_available = check_filter_availability(r);
            }

            // Request is ready to be dispatched
            dispatch(r);
        }
    }

    close(requests_pipe);
    close(status_pipe);
    return 0;
}
