#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

int running = 1;
int got_command = 0;// 1 cand primim sigusr1

void handle_usr1(int signum) {// cand monitor primeste sigusr1 seteaza got_command = 1
    got_command = 1;
}// indica facptul ca treasure_hub a scris cv in monitor_cmd.txt 

void handle_usr2(int signum) {// daca monitor primeste sigusr2 inseamna ca hub cere oprirea
    running = 0;
}

int main() {
    struct sigaction sa_usr1, sa_usr2;
    
    sa_usr1.sa_handler = handle_usr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = 0;
    sigaction(SIGUSR1, &sa_usr1, NULL);

    sa_usr2.sa_handler = handle_usr2;
    sigemptyset(&sa_usr2.sa_mask);
    sa_usr2.sa_flags = 0;
    sigaction(SIGUSR2, &sa_usr2, NULL);

    printf("Monitor started (PID: %d)\n", getpid());

    while (running) {
        pause(); // asteapta semnale

        if (got_command) {// daca primim o comanda o procesam
            got_command = 0;
        // citim comanda din fisier
            FILE *f = fopen("monitor_cmd.txt", "r");
            if (!f) continue;

            char cmd[256];
            fgets(cmd, sizeof(cmd), f);
            fclose(f);

            if (strncmp(cmd, "list_hunts", 10) == 0) {
                system("ls hunts/");
            } else if (strncmp(cmd, "list_treasures", 14) == 0) {
                char hunt_id[100];
                sscanf(cmd, "list_treasures %s", hunt_id);
                char full[200];
                snprintf(full, sizeof(full), "./treasure_manager --list %s", hunt_id);
                system(full);
            } else if (strncmp(cmd, "view_treasure", 13) == 0) {
                char hunt_id[100], tid[100];
                sscanf(cmd, "view_treasure %s %s", hunt_id, tid);
                char full[256];
                snprintf(full, sizeof(full), "./treasure_manager --view %s %s", hunt_id, tid);
                system(full);
            }
        }
    }
    // iesirea
    usleep(1000000); // intarziere
    printf("Monitor exiting\n");
    return 0;
}