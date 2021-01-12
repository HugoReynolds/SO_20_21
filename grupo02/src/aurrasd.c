/* Filename: fifoserver_twoway.c */
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "structures.h"

#define REQUESTS_PIPE "../tmp/requests_pipe"
#define STATUS_PIPE "../tmp/status_pipe"

int n_filters=0;
Filter* filter_array;

ssize_t readln(int fd, char* buf, size_t nbyte){
    // we assume that the buf is as big as nbytes
    int limite=0;
    char* read_buf = malloc(sizeof(char));
    while (read(fd,read_buf,1)>0 && limite < nbyte && (*(char*) read_buf)!='\n'){
        memcpy(&buf[limite], read_buf, 1);
        limite++;
    }
    printf("readln, Buf: %s\n", buf);
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
    printf("n_filters: %d\n", n_filters);
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

    for (int it = 0; it < n_filters; it++) printf("Name: %s, Command: %s, Quantity: %d\n", filter_array[it].name, filter_array[it].command, filter_array[it].quantity);

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

int check_filter_availability(char** transformations) {
    // returns 0 if all filters are available, if not, returns different than 0
    int filters_available = 1;
    for(int it = 0; it < transformations; it++){
        // iterate transformations array
        for(int i=0; i < n_filters; i++) {
            // iterate available filters array
            if(!strcmp(filter_array[i].name, transformations[it])) {
                // found the desired transformation in the available filters array
                if (filter_array[i].in_use == filter_array[i].quantity) return -1;
            }
        }
    }

    return 0;
}

void dispatch(Request r) {
    // allocate resources
    for (int it = 0; it < r.n_transformations; it++) {
        
    }
}

void reverse_string(char *);
int main(int argc, char* argv[]) {
    // Variables initialization
    int server_pipes[3][2];
    int mkf = mkfifo(REQUESTS_PIPE, 0666);
    printf("mkf: %d\n", mkf);
    int requests_pipe = open(REQUESTS_PIPE, O_RDWR);
    printf("reqs: %d\n", requests_pipe);

    mkfifo(STATUS_PIPE, 0666);
    int status_pipe = open(STATUS_PIPE, O_RDWR);

    printf("Req: %d, Status: %d\n", requests_pipe, status_pipe);

    int fd;
    char readbuf[80];
    char end[10];
    char s[256];
    int to_end;
    int read_bytes;
    int fconfig;

    // Server Setup
    // Filter Management
    n_filters = filter_counter() + 1;

    filter_array = malloc(n_filters * sizeof(Filter));

    get_filters(n_filters);
    
    //printf("chars read: %ld \n",read(fd,&r,sizeof(r)));

    //printf("size: %d\n", sizeof(r));    
    
    Request r;
    int readd_bytes = 0;
    while((readd_bytes = read(requests_pipe, &r, sizeof(Request))) > 0) {
        //printf("id_file: %s\nr.destfile: %s \nr.ntransformations: %d \n",r.id_file,r.dest_file,r.n_transformations);
        // Filter Verification - verification se em ou nao transformacoes
        
        int filters_exist = 1;
        for(int it = 0; it < r.n_transformations && filters_exist; it++){
            printf("filter: %s esta no indice %d \n",r.transformations[it],get_filter_index(r.transformations[it]));
            if (get_filter_index(r.transformations[it]) == -1) filters_exist = 0;
        }

        if (!filters_exist) {
            // One of the filters doesn't exist 
            // Todo
        }
        else {
            // All of the filters exist
            // We now run check__filter_availability to see if all filters are available
            // Returns 0 if they are, -1 otherwise

            int filters_available = check_filter_availability(r.transformations);

            while(filters_available) {
                // this will execute if some of the filters are not available
                sigwait();
                filters_available = check_filter_availability(r.transformations);
            }

            dispatch();

        }

    }

    close(requests_pipe);
    close(status_pipe);
    return 0;
}

void reverse_string(char *str) {
   int last, limit, first;
   char temp;
   last = strlen(str) - 1;
   limit = last/2;
   first = 0;
   
   while (first < last) {
      temp = str[first];
      str[first] = str[last];
      str[last] = temp;
      first++;
      last--;
   }
   return;
}