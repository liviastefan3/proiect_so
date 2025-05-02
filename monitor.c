#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<dirent.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<signal.h>
int running=1;
void handle_signal(int sig){
    FILE *f = fopen("command.txt","r");
    if(f == NULL) {
        perror("failed to open file");
        return;
    }
    char buffer[256];
    fgets(buffer,sizeof(buffer),f);
    fclose(f);
    if (strcmp(buffer,"list_hunts\n") == 0) {
        printf("[Monitor] Hunts list: Hunt1 (3 treasures), Hunt 2 (2 treasures)\n");
    } else if (strcmp(buffer,"list_treasures\n") == 0){
        printf("[Monitor] TReasures in Hunt1: Gold, Diamond, Ruby\n");
    } else if (strcmp(buffer,"stop_monitor\n")==0) {
        printf("[Monitor] Shutting down\n");
        usleep(2000000);//delay to simulate slow shutdown
        running=0;
    } 
}
int main() {
    struct sigaction sa;
    sa.sa_handler=handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags=0;
    sigaction(SIGUSR1, &sa, NULL);
    printf("[Monitor] Ready and waiting for commands\n");
    while(running){
        pause(); //wait for signal
    }
    printf("[Monitor] Exiting\n");
    return 0;
}
