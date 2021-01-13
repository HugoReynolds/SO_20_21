#include <unistd.h>

void main() {
    execl("../bin/aurrasd-filters/aurrasd-gain-half", "../bin/aurrasd-filters/aurrasd-gain-half", "../samples/sample-1.m4a", "output1.mp3", NULL);
}