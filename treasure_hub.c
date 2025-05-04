#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

pid_t monitor_pid = -1; // stocheaza pid ul monitorului
int monitor_active = 0; // 1 daca monitorul ruleaza, 0 altfel

void sigchld_handler(int sig) {// cand un proces fiu se termina,sistemul trimite semnalul SIGCHLD parintelui
    int status;
    waitpid(monitor_pid, &status, 0);// colecteaza starea procesului
    monitor_active = 0;
    printf("Monitor ended with status %d\n", status);
}

void send_command(const char *cmd) {// comunica cu procesul monitor
    FILE *f = fopen("monitor_cmd.txt", "w");
    if (!f) {
        perror("fopen");
        return;
    }
    fprintf(f, "%s\n", cmd);
    fclose(f);
    kill(monitor_pid, SIGUSR1);
}

int main() {
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGCHLD, &sa, NULL);

    char input[256];// citire comenzi de la tastaura

    while (1) {
        printf("> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0) {
            if (monitor_active) {
                printf("Monitor already running\n");
                continue;
            }

            monitor_pid = fork();
            if (monitor_pid == 0) {
                execl("./monitor", "monitor", NULL);
                perror("execl");
                exit(1);
            }

            monitor_active = 1;
        } else if (strcmp(input, "stop_monitor") == 0) {
            if (!monitor_active) {
                printf("No monitor running\n");
            } else {
                kill(monitor_pid, SIGUSR2);
            }
        } else if (strcmp(input, "exit") == 0) {
            if (monitor_active) {
                if (kill(monitor_pid, 0) == -1) {
                    monitor_active = 0; // deja a iesit
                    break;
                }
                printf("Monitor still running. Use stop_monitor first\n");
            } else {
                break;
            }            
        } else if (strncmp(input, "list_hunts", 10) == 0 ||
                   strncmp(input, "list_treasures", 14) == 0 ||
                   strncmp(input, "view_treasure", 13) == 0) {
            if (!monitor_active) {
                printf("Monitor not running\n");
            } else {
                send_command(input);
            }
        } else {
            printf("Unknown command\n");
        }
    }

    return 0;
}