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

#define REQUESTS_PIPE "../tmp/requests_pipe"

int flag = 1;  // main while cycle
int n_filters = 0;  // number of filters of the filter_array
Filter* filter_array;

// Total Tasks
int total_tasks = 0;   // task counter

// In Progress Tasks
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

char *my_itoa(int num, char *str)  // itoa had problems so i looked for an implementation
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
    int filter_file = open(conf_dir,O_RDONLY);
    if (filter_file == -1) printf("Something went wrong");
    else {
        char* buf = malloc(sizeof(char));
        while(read(filter_file, buf, 1) > 0) {
            if (!strcmp(buf,"\n")) lines++;
        }
    }
    return lines / 3;
}



void get_filters() {
    // reads from conf file and fills filter_array
    char* buf = malloc(256*sizeof(char));
    int filter_file = open(conf_dir,O_RDONLY);

    for (int it = 0; it < n_filters; it++) {
        // on every iteration reads the filter's attributes from the conf file 
        // name
        memset(buf, 0, 256);
        readln(filter_file, buf, 256);
        strcpy(filter_array[it].name, buf);
        
        // command
        memset(buf, 0, 256);
        readln(filter_file, buf, 256);
        strcpy(filter_array[it].command, buf);

        // instances
        memset(buf, 0, 256);
        readln(filter_file, buf, 256);
        filter_array[it].quantity=atoi(buf);
        memset(buf, 0, 256);

        // in use
        filter_array[it].in_use = 0;
    }

    printf("--- Available Filters ---\n\n");
    for (int it = 0; it < n_filters; it++) printf("Filter nr: %d. Name: %s, Quantity: %d\n", it, filter_array[it].name, filter_array[it].quantity);

    free(buf);
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
    for(int it = 0; it < r.n_transformations; it++){
        // compares in use instances of a requested filter and checks if one more unity can be allocated
        if (filter_array[get_filter_index(r.transformations[it])].in_use == filter_array[get_filter_index(r.transformations[it])].quantity) return -1;
    }

    return 0;
}

void print_filters() {
    printf("\n--- Instances of filters in use ---\n\n");
    for (int it = 0; it < n_filters; it++) {
        printf("%s: (%d/%d) (in use / total)\n", filter_array[it].name, filter_array[it].in_use, filter_array[it].quantity);
    }
    printf("\n++++++++++++++++++++++++++++++++++\n\n");
}


void signal_handler(int sig) {
    //function that handles signals
    pid_t pid = wait(NULL);
    //int status;
    //pid_t pid = waitpid(0, &status, WNOHANG);
    //if (WIFEXITED(status)) printf("PID: %d finished with status %d\n", pid, status);
    
    InProgress* curr = in_progress_list->head;
    int curr_index = 0;

    if (curr == NULL) {
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
                    printf("\n--- Released Resources that were allocated to PID: %d ---\n\n", curr->pid_array[it]);
                    filter_array[filter_index].in_use--;
                }

                // increments number of done transformtions 
                curr->done_transformations++;
                printf("--- Task [%d]: %d/%d transformations done.--- \n\n", curr->task_nr, curr->done_transformations, curr->n_transformations);

                if (curr->done_transformations == curr->n_transformations) {
                    // all transformations were completed so the request is done
                    // sends information to the client to inform that his request is finished 

                    printf("\n--- Task [%d] is finished! --- \n\n", curr->task_nr);
                    
                    // variable initialization
                    char my_pipe[24];
                    char my_pid[6];
                    char task_nr[6];

                    // data initialization
                    my_itoa(curr->pid, my_pid);
                    my_itoa(curr->task_nr, task_nr);
                    strcpy(my_pipe, "../tmp/pid");
                    strcat(my_pipe + 10,  my_pid);

                    Status_Reply sr;
                    strcpy(sr.msg[0], "\n--- Task [");
                    strcat(sr.msg[0] + 6, task_nr);
                    strcat(sr.msg[0] + 6 + strlen(task_nr), "] is done. Thanks for using our service. ---\n\n");
                    sr.lines = 1;

                    // opens file descriptor to client's pipe and sends the information that the request is finished 

                    int request_pipe = open(my_pipe, O_WRONLY);
                    write(request_pipe, &sr, sizeof(Status_Reply));
                    close(request_pipe);

                    // removes the task from the in_progress_list

                    removeIndex(in_progress_list, curr_index);
                }
                return;
            }
        }
        curr = curr->next;
        curr_index++;
    }
}

void dispatch(Request r) {
    // allocate resources
    for (int it = 0; it < r.n_transformations; it++) {
        // for each transformation, increments the number of instances for the corresponding filter
        filter_array[get_filter_index(r.transformations[it])].in_use++;
        print_filters();
    }

    // creation of InProgress task to be added to in_progress_list
    InProgress ip;
    ip.next = NULL;
    ip.task_nr = ++total_tasks;
    ip.done_transformations = 0;
    ip.pid = r.pid;
    ip.n_transformations = r.n_transformations;
    strcpy(ip.origin_file,r.id_file);
    strcpy(ip.dest_file,r.dest_file);


    for (int it = 0; it < r.n_transformations; it++) {
        // for each transformation ...
        strcpy(ip.task_array[it], r.transformations[it]);
        int filter_index = get_filter_index(ip.task_array[it]);

        // format task string with dir
        char* transf_with_dir = malloc((strlen(filter_array[filter_index].command) + strlen(trans_dir)) * sizeof(char));
        strcpy(transf_with_dir, trans_dir);
        strcpy(transf_with_dir + strlen(trans_dir), filter_array[filter_index].command);
        if (r.n_transformations == 1) {
            // if it's only one transformation
            // child opens both files for stdin and stdout directly
            ip.pid_array[it] = fork();

            if (ip.pid_array[it] == 0) {
                // child
                // redirect the file to the stdin
                int open_file = open(ip.origin_file, O_RDONLY);
                dup2(open_file, 0);
                close(open_file);
                
                // redirect the stdout to the dest_file
                int dest_file = open(ip.dest_file, O_CREAT | O_APPEND | O_RDWR, 0644);
                dup2(dest_file, 1);
                close(dest_file);

                execl(transf_with_dir, transf_with_dir, NULL);
                _exit(-1);
            }

        }
        else {
            // pipes
            if (it > 1) {
                // close obsolete pipes, otherwise, childs lock up
                close(ip.pipe_matrix[it-2][0]);
                close(ip.pipe_matrix[it-2][1]);
            }

            if (it < (r.n_transformations - 1)) {
                // not the last iteration
                pipe(ip.pipe_matrix[it]);
            }

            // fork

            ip.pid_array[it] = fork();
            if (!ip.pid_array[it]) {
                // child
                if (!it) {
                    // first child
                    // closing reading end
                    close(ip.pipe_matrix[it][0]);

                    // redirect file's content to stdin
                    int origin_file = open(ip.origin_file, O_RDONLY);
                    dup2(origin_file, 0);
                    close(origin_file);

                    // redirect writing end
                    dup2(ip.pipe_matrix[it][1],1);
                    close(ip.pipe_matrix[it][1]);

                    execl(transf_with_dir, transf_with_dir, NULL);
                    _exit(1);
                }
                else{ 
                    if (it == ip.n_transformations-1) {
                        // last child
                        // redirect reading end
                        dup2(ip.pipe_matrix[it-1][0],0);
                        close(ip.pipe_matrix[it-1][0]);

                        // close writing end
                        close(ip.pipe_matrix[it-1][1]);
                        

                        // redirect stdout to file
                        int dest_file = open(ip.dest_file, O_CREAT | O_APPEND | O_RDWR, 0644);
                        dup2(dest_file, 1);
                        close(dest_file);

                        execl(transf_with_dir, transf_with_dir, NULL);
                        _exit(1);
                    }
                    else {
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

                        execl(transf_with_dir, transf_with_dir, NULL);
                        _exit(1);
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

    printf("\n+++ Added Task [%d] to InProgess +++\n\n", ip.task_nr);
    for(int it = 0; it < ip.n_transformations; it++) printf("PID: %d -> Filter %s\n", ip.pid_array[it], ip.task_array[it]);
    printf("\n");
}

void status_replier(int pid) {
    // Sends current server status to client
    char my_pipe[24];
    char my_pid[6];
    my_itoa(pid, my_pid);
    strcpy(my_pipe, "../tmp/pid");
    strcat(my_pipe + 10,  my_pid);
    
    Status_Reply rep;

    int rep_lines = 0;

    InProgress* ip = NULL;        

    if (in_progress_list->tasks_in_progress > 0) ip = getIndex(in_progress_list, 0);


    // Listing tasks
    while(ip != NULL && rep_lines < 9) {

        // variable declaration
        char msg[1024];
        char task_nr[4];
        char origin_file[256];
        char dest_file[256];
        int msg_len = 0;

        // value allocation

        my_itoa(ip->task_nr, task_nr);
        strcpy(origin_file, ip->origin_file);
        strcpy(dest_file, ip->dest_file);

        // message assembly

        strcpy(msg, "Task #");
        msg_len = 6;

        strcat(msg + msg_len, task_nr);
        msg_len += strlen(task_nr);

        strcat(msg + msg_len, ": transform ");
        msg_len += 12;

        strcat(msg + msg_len, origin_file);
        msg_len += strlen(origin_file);

        strcat(msg + msg_len, " ");
        msg_len++;

        strcat(msg + msg_len, dest_file);
        msg_len += strlen(dest_file);

        strcat(msg + msg_len, " ");
        msg_len++;
        
        for (int it = 0; it < ip->n_transformations; it++) {
            // task addition
            strcat(msg + msg_len, ip->task_array[it]);
            msg_len += strlen(ip->task_array[it]);
            
            strcat(msg + msg_len, " ");
            msg_len++;    
        }

        strcpy(rep.msg[rep_lines++], msg);
        ip = ip->next;
    }

    // listing filters
    for (int it = 0; it < (20 - rep_lines) && it < n_filters; it++) {
        
        // variable declaration
        
        char msg[1024];
        char in_use[3];
        char total[3];
        int msg_len = 0;

        // value allocation
        my_itoa(filter_array[it].in_use, in_use);
        my_itoa(filter_array[it].quantity, total);

        // message assembly

        strcpy(msg + msg_len, "Filter ");
        msg_len += 7;

        strcpy(msg + msg_len, filter_array[it].name);
        msg_len += strlen(filter_array[it].name);

        strcpy(msg + msg_len, ": ");
        msg_len += 2;

        strcpy(msg + msg_len, in_use);
        msg_len += strlen(in_use);

        strcpy(msg + msg_len, "/");
        msg_len += 1;

        strcpy(msg + msg_len, total);
        msg_len += strlen(total);

        strcpy(msg + msg_len, " (in use/total).");
        msg_len += 16;

        strcpy(rep.msg[rep_lines++], msg);

    }

    // pid addtion
    // variable declaration
    
    char msg[1024];

    strcpy(msg, "pid: ");
    strcpy(msg + 5, my_pid);
    strcpy(rep.msg[rep_lines++],msg);

    rep.lines = rep_lines;

    for (int it=0; it < rep_lines; it++) printf("Msg [%d]: %s\n", it, rep.msg[it]);

    int pipe = open(my_pipe, O_WRONLY);
    printf(" --- Sending status to %s. FD: %d ---\n\n", my_pipe, pipe);
    write(pipe, &rep, sizeof(Status_Reply));
    close(pipe);
    _exit(0);
}

int main(int argc, char* argv[]) {
    if (argc != 3) return 1;
    else {
        conf_dir = strdup(argv[1]);
        trans_dir = strdup(argv[2]);
    }

    // Variables initialization
    // Creates and opens Requests_pipe
    mkfifo(REQUESTS_PIPE, 0666);
    int requests_pipe = open(REQUESTS_PIPE, O_RDWR);

    if (requests_pipe == -1) return 0;

    printf("\n---PID Main: %d---\n\n", getpid());

    // Server Setup
    // Signal Handlers
    signal(SIGCHLD, signal_handler);
   
    // Filter Management
    n_filters = filter_counter() + 1;

    filter_array = malloc(n_filters * sizeof(Filter));
    in_progress_list = list_init();

    // reads from config file and populates filter_array
    get_filters();
    
    while(flag) {
        Request r;
        read(requests_pipe, &r, sizeof(Request));
        if (!r.code) {
            // Status Request
            // deploy status receiver
            
            if (fork() == 0) {
                // child
                status_replier(r.n_transformations);
            }
        }
        else {
            // Transformation Request
            // verify if the requested filters exist
            int filters_exist = 1;
            for(int it = 0; it < r.n_transformations && filters_exist; it++) if (get_filter_index(r.transformations[it]) == -1) filters_exist = 0;

            if (!filters_exist) {
                // One of the filters doesn't exist 
                // variable initialization
                char my_pipe[24];
                char my_pid[6];

                // data initialization
                my_itoa(r.pid, my_pid);
                strcpy(my_pipe, "../tmp/pid");
                strcat(my_pipe + 10,  my_pid);

                Status_Reply sr;
                strcpy(sr.msg[0], "\nOne of the filters you requested doesn't exist. Please reformulate it.\n");
                sr.lines = 1;

                // opens file descriptor to client's pipe and sends the information that the request is finished 

                int request_pipe = open(my_pipe, O_WRONLY);
                write(request_pipe, &sr, sizeof(Status_Reply));
                close(request_pipe);
            }
            else {
                // All of the filters exist
                // We now run check__filter_availability to see if all filters are available
                // Returns 0 if they are, -1 otherwise
                int filters_available = check_filter_availability(r);
                while(filters_available) {
                    // this will execute if some of the filters are not available
                    sleep(5);
                    filters_available = check_filter_availability(r);
                }

                // Request is ready to be dispatched
                dispatch(r);
            }
        }
    }

    close(requests_pipe);
    return 0;
}
