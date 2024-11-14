#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
int main()
{
    const int idxmax = 50;
    int idx = 1;
    pid_t pid;
    for(idx = 1 ; idx <= idxmax ; idx++) {
        printf("#%d forking...\n", idx);
        fflush(stdout);
        pid = fork();
        if (pid > 0) {
            printf("#%d fork success, waiting...\n", idx);
            fflush(stdout);
            waitpid(pid, NULL, 0);
            printf("#%d exiting...\n", idx);
            break;
        }
        else if (pid < 0) {
            printf("#%d fork failed, error is %s\n", idx, strerror(errno));
            printf("#%d exiting...\n", idx);
            break;
        }
    }
    fflush(stdout);
    return 0;
}
