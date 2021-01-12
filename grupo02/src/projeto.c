#include <stdio.h>
#include <unistd.h>


int main(void){
    
    char *binaryPath = "/bin/ls";
    char *arg1 = "-lh";
    char *arg2 = "/home";


    int a = execl(binaryPath,binaryPath,arg1,arg2,NULL);
    printf("a: %d",a);

    return 0;
}    