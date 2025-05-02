#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<dirent.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<signal.h>
#include<sys/wait.h>
 pid_t monitor_pid = -1;
 int monitor_exiting = 0;
 void handle_sigchild(int sig)
 {
    int status;
    waitpid(monitor_pid, &status, 0);
    printf("[Hub] Monitor exited with status &d\n",WEXITSTATUS(status));
    monitor_pid=-1;
    monitor_exiting=0;
 }