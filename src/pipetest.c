#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {
    char* trans1 = "../bin/aurrasd-filters/aurrasd-echo";
    char* trans2 = "../bin/aurrasd-filters/aurrasd-gain-double";
    char* trans3 = "../bin/aurrasd-filters/aurrasd-tempo-double";
    char* trans4 = "../bin/aurrasd-filters/aurrasd-tempo-half";
    char* og_file = "../samples/sample-1.m4a";
    char* dest_file = "../samples/output78686.m4a";
    
    int n_of_transformations = 4;

    int pid_array[n_of_transformations];
    int pipe_matrix[n_of_transformations-1][2];

    for (int it = 0; it < n_of_transformations; it++) {
        // fechar pipes obsoletos
        if (it > 1) {
            close(pipe_matrix[it-2][0]);
            close(pipe_matrix[it-2][1]);
        }

        // [0] - p0 -> [1] - p1 -> [2] - p2 -> [3]
        
        // abrir pipe
        if (it < (n_of_transformations - 1)) pipe(pipe_matrix[it]);

        // criar filho
        pid_array[it] = fork();
        if (!pid_array[it]) {
            // filho

            if (!it) {
                // primeiro filho
                // fechar reading end
                close(pipe_matrix[it][0]);

                // abrir ficheiro a processar e redirecionar para o stdin
                int file = open(og_file, O_RDONLY);
                printf("First child. File: %d. PID: %d.\n", file, getpid());
                dup2(file,0);
                close(file);

                // redirecionar writing end e fecjar
                dup2(pipe_matrix[it][1],1);
                close(pipe_matrix[it][1]);

                execl(trans1, trans1, NULL);
                exit(1);
            }
            if (it == (n_of_transformations - 1)) {
                // último filho
                printf("Last Child. PID: %d.\n", getpid());                

                // fechar writing end do pipe anterior
                close(pipe_matrix[it-1][1]);

                // redirecionar reading end do pipe anterior para o stdin
                dup2(pipe_matrix[it-1][0],0);
                close(pipe_matrix[it-1][0]);

                // abrir ficheiro final e redirecionar stdout para lá
                int file = open(dest_file, O_WRONLY | O_APPEND | O_CREAT, 0644);
                printf("--- Last child, opened %s with fd: %d ---\n\n", dest_file, file);
                dup2(file,1);
                close(file);

                execl(trans3, trans3, NULL);
                exit(1);
            }
            else {
                // filhos do meio
                printf("Middle Child. PID: %d.\n", getpid());

                // fechar writing end do pipe anterior
                close(pipe_matrix[it-1][1]);

                // fechar reading end do pipe seguinte
                close(pipe_matrix[it][0]);

                // redirecionar reading end do pipe anterior para o stdin
                dup2(pipe_matrix[it-1][0],0);
                close(pipe_matrix[it-1][0]);

                // redirecionar stdout para o writing end do pipe seguinte
                dup2(pipe_matrix[it][1], 1);
                close(pipe_matrix[it][1]);

                execl(trans2, trans2, NULL);
                exit(1);
            }
        }
        else {
            // pai
        }
    }

    close(pipe_matrix[n_of_transformations - 2][0]);
    close(pipe_matrix[n_of_transformations - 2][1]);

    for (int it = 0; it < n_of_transformations ; it++) {
        printf("\n---Waiting---\n");
        pid_t child_pid = wait(NULL);
        printf("Child with pid %d is finished\n", child_pid);
    }

    return 0;
}