/* Filename: fifoclient_twoway.c */
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "structures.h"


#define REQUESTS_PIPE "../tmp/requests_pipe"
#define STATUS_PIPE "../tmp/status_pipe"

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
    printf("Pipe: %s. FD: %d\n", my_pipe, pipe_fd);
    srand(time(NULL));
    char* filter_array[] = {
        "eco",
        "alto",
        "baixo",
        "lento",
        "rapido"
    };
    
    switch (argc){
        case 1:
            printf("./aurras transform input-filename output-filename filter-id-1 filter-id-2 ...\n");
            return 0;
            break;
        case 2:
            if(strcmp("status",argv[1])==0){
                Request r;
                r.code = 0;
                r.n_transformations = getpid();
                write(requests_pipe, &r, sizeof(Request));

                Status_Reply sr;
                printf("Before read\n");
                int read_bytes = read(pipe_fd, &sr, sizeof(Status_Reply));
                printf("After read. Bytes: %d\n", read_bytes);
                for (int it = 0; it < sr.lines; it++) printf("%s\n", sr.msg[it]);
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
                
                // gerador de pedidos aleatório
                for (int it = 0; it < 50; it++) {
                    Request request;
                    request.code = 1;
                    memset(&request.id_file, 0, 128);
                    strcpy(request.id_file, argv[2]);
                    memset(&request.dest_file, 0, 128);
                    strcpy(request.dest_file, argv[3]);
                    request.n_transformations=(rand() % 3 + 2);
                    printf("--- Task [%d]: Nº of trans: %d ---\n", it+1, request.n_transformations);
                    int trans_it=0;
                    int check_array[] = {0,0,0,0,0};
                    for (int i = 0; i < request.n_transformations; i++) {
                        int index_of_trans = (rand() % request.n_transformations + 0 + 1);
                        while(check_array[index_of_trans]) {
                            index_of_trans = (rand() % request.n_transformations + 0 + 1);
                        }
                        
                        // o indice index_of_trans ainda está a 0, o que significa, que a trans não foi escolhida
                        check_array[index_of_trans] = 1;
                        strcpy(request.transformations[trans_it], filter_array[index_of_trans]);
                        printf("A transf: %s, foi selecionada, para o indice: %d\n", filter_array[index_of_trans], trans_it);
                        trans_it++;
                    }
                    printf("\n");

                    write(requests_pipe,&request,sizeof(Request));
                    sleep(rand() % 5 + 3 + 1);
                }
                
                /*
                printf("Inside default\n");
                Request request;
                memset(&request.id_file, 0, 128);
                strcpy(request.id_file, argv[2]);
                memset(&request.dest_file, 0, 128);
                strcpy(request.dest_file, argv[3]);
                request.n_transformations=argc-4;
                int trans_it=0;
                
                for(int i=4; i<argc; i++ && trans_it++){
                    strcpy(request.transformations[trans_it], argv[i]);
                }
                
                trans_it=0;

                for(trans_it=0; trans_it<request.n_transformations; trans_it++) {
                    printf("transformation n%d: %s\n",trans_it+1,request.transformations[trans_it]);   
                }
                printf("size: %ld \n",sizeof(request));

                write(requests_pipe,&request,sizeof(Request));
                
                sleep(2);

                printf("2nd send\n");

                Request r2;
                strcpy(r2.id_file, "Asdasdas2");
                strcpy(r2.dest_file, "Dest");
                r2.n_transformations=argc-4;
                trans_it=0;
                
                for(int i=4; i<argc; i++ && trans_it++){
                    strcpy(r2.transformations[trans_it], argv[i]);
                }
                
                trans_it=0;

                for(trans_it=0; trans_it<r2.n_transformations; trans_it++) {
                    printf("transformation n%d: %s\n",trans_it+1,r2.transformations[trans_it]);   
                }

                printf("r2.file: %s, r2.n_transformations: %d\n", r2.id_file, r2.n_transformations);

                write(requests_pipe,&r2,sizeof(Request));
                */
                //free_request(request);
                close(requests_pipe);

                return 0;
            } else {
                printf("you have incorrectly use the program, use it like this: ./aurras transform input-filename output-filename filter-id-1 filter-id-2 ...\n");
                return 0;
            }

            

            break;
                 
    }

   return 0;
}