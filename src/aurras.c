#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "structures.h"

#define REQUESTS_PIPE "../tmp/requests_pipe"

void free_request(Request r){
    for (int i=0; i<r.n_transformations; i++){
        free(r.transformations[i]);
    }
}

char *my_itoa(int num, char *str) {
    if(str == NULL)
    {
            return NULL;
    }
    sprintf(str, "%d", num);
    return str;
}

int main(int argc, char* argv[]) {
    int requests_pipe = open(REQUESTS_PIPE, O_CREAT|O_WRONLY);
    char my_pipe[24];
    char my_pid[6];
    strcpy(my_pipe, "../tmp/pid");
    strcat(my_pipe + 10, my_itoa(getpid(), my_pid));
    mkfifo(my_pipe, 0644);
    int pipe_fd = open(my_pipe, O_RDWR);
    
    switch (argc){
        case 1:
            printf("You have incorrectly used the program, its use is as follows: ./aurras transform input-filename output-filename filter-id-1 filter-id-2 ...\n");
            return 0;
            break;
        case 2:
            if(strcmp("status",argv[1])==0){
                // create and send status request to server
                Request r;
                r.code = 0;
                r.n_transformations = getpid();
                write(requests_pipe, &r, sizeof(Request));

                // read reply from server and print it
                Status_Reply sr;
                read(pipe_fd, &sr, sizeof(Status_Reply));
                for (int it = 0; it < sr.lines; it++) printf("%s\n", sr.msg[it]);

                // close pipes
                close(pipe_fd);
                close(requests_pipe);
                return 0;
            } else {
                printf("You have incorrectly used the program, its use is as follows: ./aurras transform input-filename output-filename filter-id-1 filter-id-2 ...\n");
                return 0;
            }
            break;
        default:
            if((strcmp("transform",argv[1])==0) && (argc>4) && (argc<14)){
                // variable initialization
                
                Request request;
                memset(&request.id_file, 0, 128);
                strcpy(request.id_file, argv[2]);
                memset(&request.dest_file, 0, 128);
                strcpy(request.dest_file, argv[3]);
                request.code = 1;
                request.pid = getpid();
                request.n_transformations=argc-4;
                int trans_it=0;
                
                for(int i=4; i<argc; i++){
                    strcpy(request.transformations[trans_it++], argv[i]);
                }
                
                trans_it=0;

                for(trans_it=0; trans_it<request.n_transformations; trans_it++) {
                    printf("Transformation [%d]: %s\n",trans_it+1,request.transformations[trans_it]);   
                }

                write(requests_pipe,&request,sizeof(Request));
                //free_request(request);

                // read reply from server and print it
                Status_Reply sr;
                read(pipe_fd, &sr, sizeof(Status_Reply));
                for (int it = 0; it < sr.lines; it++) printf("%s\n", sr.msg[it]);

                // close pipes
                close(pipe_fd);
                close(requests_pipe);
                return 0;
            }

            if (strcmp("test",argv[1])==0) {
                // random requests sender
                // varoable initialization
                srand(time(NULL));
                char* filter_array[] = {
                    "eco",
                    "alto",
                    "baixo",
                    "lento",
                    "rapido"
                };

                int n_of_requests = atoi(argv[2]);

                int child_pid = fork();
                if (child_pid == 0) {
                    // child - gonna read replies from server
                    int received_requests = 0;
                    while(received_requests < n_of_requests) {
                        // read reply from server and print it
                        Status_Reply sr;
                        read(pipe_fd, &sr, sizeof(Status_Reply));
                        for (int it = 0; it < sr.lines; it++) printf("%s\n", sr.msg[it]);
                        received_requests++;
                    }
                    _exit(0);
                }

                for (int it = 0; it < n_of_requests; it++) {
                    Request request;
                    request.code = 1;
                    memset(&request.id_file, 0, 256);
                    memset(&request.dest_file, 0, 256);
                    strcpy(request.id_file, "../samples/sample-1.m4a");
                    strcat(request.dest_file, "../samples/outputs/vitor_feliz_ano_");
                    int msg_len = strlen("../samples/outputs/vitor_feliz_ano_");
                    
                    char itt[3];
                    my_itoa(it, itt);

                    strcat(request.dest_file + msg_len, itt);
                    msg_len += strlen(itt);

                    request.pid = getpid();
                    request.n_transformations=(rand() % (5 - 2 + 1) + 2);
                    printf("--- Task [%d]: Nº of trans: %d ---\n\n", it+1, request.n_transformations);
                    int trans_it=0;
                    int check_array[] = {0,0,0,0,0};
                    for (int i = 0; i < request.n_transformations; i++) {
                        int index_of_trans = (rand() % (4 - 0 + 1) + 0);
                        while(check_array[index_of_trans]) {
                            index_of_trans = (rand() % (4 - 0 + 1) + 0);
                        }
                        
                        strcat(request.dest_file + msg_len, "_");
                        msg_len++;

                        strcat(request.dest_file + msg_len, filter_array[index_of_trans]);
                        msg_len += strlen(filter_array[index_of_trans]);

                        // o indice index_of_trans ainda está a 0, o que significa, que a trans não foi escolhida
                        check_array[index_of_trans] = 1;
                        strcpy(request.transformations[trans_it], filter_array[index_of_trans]);
                        printf("Transformation[%d]: %s\n", trans_it, filter_array[index_of_trans]);
                        trans_it++;
                    }
                    
                    printf("\n");
                    strcat(request.dest_file + msg_len, "_");
                    msg_len++;

                    time_t rawtime;
                    struct tm * timeinfo;
                    time(&rawtime);
                    timeinfo = localtime(&rawtime);
                    // variable initialization
                    char* hours = malloc(3*sizeof(char));
                    char* minutes = malloc(3*sizeof(char));
                    char* secs = malloc(3*sizeof(char));
                    

                    // data initialization
                    my_itoa(timeinfo->tm_hour, hours);
                    my_itoa(timeinfo->tm_min, minutes);
                    my_itoa(timeinfo->tm_sec, secs);
                    
 
                    strcat(request.dest_file + msg_len, hours);
                    msg_len += strlen(hours);

                    strcat(request.dest_file + msg_len, ":");
                    msg_len++;

                    strcat(request.dest_file + msg_len, minutes);
                    msg_len += strlen(minutes);

                    strcat(request.dest_file + msg_len, ":");
                    msg_len++;

                    strcat(request.dest_file + msg_len, secs);
                    msg_len += strlen(secs);

                    strcat(request.dest_file + msg_len, ".mp3");


                    write(requests_pipe,&request,sizeof(Request));
                    free(hours);
                    free(minutes);
                    free(secs);
                    sleep(rand() % 5 + 3 + 1);
                }

                // wait for child
                wait(NULL);

                close(requests_pipe);
                return 0;
            }
            
            else {
                printf("You have incorrectly used the program, its use is as follows: ./aurras transform input-filename output-filename filter-id-1 filter-id-2 ...\n");
                return 0;
            }

            

            break;
                 
    }

   return 0;
}