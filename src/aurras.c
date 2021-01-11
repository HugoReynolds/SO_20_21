/* Filename: fifoclient_twoway.c */
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "structures.h"


#define FIFO_FILE "../tmp/requests_pipe"

void free_request(Request r){
    for (int i=0; i<r.n_transformations; i++){
        free(r.transformations[i]);
    }
}


int main(int argc, char* argv[]) {
   int fd;
   int end_process;
   int stringlen;
   int read_bytes;
   char readbuf[80];
   char end_str[5];
   printf("FIFO_CLIENT: Send messages, infinitely, to end enter \"end\"\n");
   fd = open(FIFO_FILE, O_CREAT|O_RDWR);
   printf("Req: %d\n", fd);
   strcpy(end_str, "end");

   
    switch (argc){
        case 1:
            printf("./aurras transform input-filename output-filename filter-id-1 filter-id-2 ...\n");
            return 0;
            break;
        case 2:
            printf ("argv[1]: %s\n",argv[1]);
            if(strcmp("status",argv[1])==0){
                printf("e'status\n");
                return 0;
            } else {
                printf("you have incorrectly use the program, use it like this: ./aurras transform input-filename output-filename filter-id-1 filter-id-2 ...\n");
                return 0;
            }
            break;
        default:

            if((strcmp("transform",argv[1])==0) && (argc>4) && (argc<14)){
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

                write(fd,&request,sizeof(request));

                
                //free_request(request);
                close(fd);
                //printf("%d\n",request.n_transformations);

                return 0;
            } else {
                printf("you have incorrectly use the program, use it like this: ./aurras transform input-filename output-filename filter-id-1 filter-id-2 ...\n");
                return 0;
            }

            

            break;
                 
    }







   /*while (1) {
      printf("Enter string: ");
      fgets(readbuf, sizeof(readbuf), stdin);
      stringlen = strlen(readbuf);
      readbuf[stringlen - 1] = '\0';
      end_process = strcmp(readbuf, end_str);
      
      //printf("end_process is %d\n", end_process);
      if (end_process != 0) {
         write(fd, readbuf, strlen(readbuf));
         printf("FIFOCLIENT: Sent string: \"%s\" and string length is %d\n", readbuf, (int)strlen(readbuf));
         read_bytes = read(fd, readbuf, sizeof(readbuf));
         readbuf[read_bytes] = '\0';
         printf("FIFOCLIENT: Received string: \"%s\" and length is %d\n", readbuf, (int)strlen(readbuf));
      } else {
         write(fd, readbuf, strlen(readbuf));
         printf("FIFOCLIENT: Sent string: \"%s\" and string length is %d\n", readbuf, (int)strlen(readbuf));
         close(fd);
         break;
      }
   }*/
   return 0;
}