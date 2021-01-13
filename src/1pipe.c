#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

int main (int argc, char* argv[]) {
    // ../bin/aurrasd-filters/aurrasd-echo < sample-1.m4a > output.mp3

    char* trans1 = "../bin/aurrasd-filters/aurrasd-echo";
    char* trans2 = "../bin/aurrasd-filters/aurrasd-gain-double";
    char* trans3 = "../bin/aurrasd-filters/aurrasd-tempo-double";
    char* og_file = "../samples/sample-1.m4a";
    char* dest_file = "../samples/output22.mp3";

    // file -> stdin
    int open_f = open(og_file, O_RDONLY);
    printf("open_f: %d\n", open_f);
    dup2(open_f,0);
    close(open_f);

    // stdout -> file
    int fd = open(dest_file, O_WRONLY | O_APPEND | O_CREAT, 0666);
    printf("fd: %d\n", fd);
    dup2(fd,1);
    close(fd);

    execl(trans1, trans1, NULL);
    exit(1);
}