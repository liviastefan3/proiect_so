#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>

#define BUF_SIZE 256
#define END_MARKER '\x04' // caracter special de final (ASCII EOT) pentru a semnala sfarsitul output ului la copil

int main() {
    int pipe_cmd[2];  // canalul hub - monitor(parintele scrie, copilul citeste)
    int pipe_out[2];  // monitor - hub(copilul scrie, parintele citeste)

    if (pipe(pipe_cmd) == -1 || pipe(pipe_out) == -1) {// creeaza 2 pipe uri si verifica daca au fost create
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork(); //creeaza un proces copil (monitor)
    if (pid == -1) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        // copil - Monitor
        close(pipe_cmd[1]);  // inchide scriere comenzi
        close(pipe_out[0]);  // inchide citire output

        dup2(pipe_cmd[0], STDIN_FILENO); // citeste comenzi prin stdin
        dup2(pipe_out[1], STDOUT_FILENO); // scrie rezultatele prin stdout

        char cmd[BUF_SIZE];
        while (fgets(cmd, sizeof(cmd), stdin)) {
            cmd[strcspn(cmd, "\n")] = 0; // scoate '\n'
            if (strlen(cmd) == 0) continue; //ignora comenzi goale

            if (strcmp(cmd, "list_hunts") == 0) { //listeaza directoarele cu hunts/  sis emnaleaza sfarsitul cu end_marker
                system("ls hunts/");
                printf("%c", END_MARKER); fflush(stdout);// am folosit fflush pt a forta trimiterea imediata a continutului din buffer ul de iesire catre pipe
            } else if (strncmp(cmd, "list_treasures", 14) == 0) {
                char hunt_id[100];
                if (sscanf(cmd, "list_treasures %s", hunt_id) != 1) {
                    printf("Error: list_treasures needs hunt_id\n%c", END_MARKER); fflush(stdout);
                    continue;
                }//extrage hunt_id verifica daca e prezent
                char full[256];
                snprintf(full, sizeof(full), "./treasure_manager --list %s", hunt_id);
                system(full);
                printf("%c", END_MARKER); fflush(stdout);
            } else if (strncmp(cmd, "view_treasure", 13) == 0) {
                char hunt_id[100], tid[100];
                if (sscanf(cmd, "view_treasure %s %s", hunt_id, tid) != 2) {
                    printf("Error: view_treasure needs hunt_id and treasure_id\n%c", END_MARKER); fflush(stdout);
                    continue;
                }
                char full[256];
                snprintf(full, sizeof(full), "./treasure_manager --view %s %s", hunt_id, tid);
                system(full);
                printf("%c", END_MARKER); fflush(stdout);
            } else if (strcmp(cmd, "exit_monitor") == 0) {
                printf("Monitor exiting...\n%c", END_MARKER); fflush(stdout);
                break;
            } else {
                printf("Unknown command in monitor: %s\n%c", cmd, END_MARKER); fflush(stdout);
            }// iese din loop deci copilul se inchide, trimite si marker ul final
        }

        exit(0);
    }

    // Parinte - treasure_hub
    close(pipe_cmd[0]); // închide citire comenzi , parintele nu citeste comenzi
    close(pipe_out[1]); // închide scriere output, parintele nu scrie output

    char input[BUF_SIZE];
    while (1) { //citirea comenzilor de la utilizator
        printf("> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "exit") == 0) {
            dprintf(pipe_cmd[1], "exit_monitor\n");

            // citește output ul până la END_MARKER
            char ch;
            while (read(pipe_out[0], &ch, 1) == 1) {
                if (ch == END_MARKER) break;
                putchar(ch);
            }

            close(pipe_cmd[1]);
            waitpid(pid, NULL, 0);
            break;
        } else if (strncmp(input, "list_hunts", 10) == 0 ||
                   strncmp(input, "list_treasures", 14) == 0 ||
                   strncmp(input, "view_treasure", 13) == 0) {

            dprintf(pipe_cmd[1], "%s\n", input);

            // citește output până la END_MARKER
            char ch;
            while (read(pipe_out[0], &ch, 1) == 1) {
                if (ch == END_MARKER) break;
                putchar(ch);
            }

        } else {
            printf("Unknown command, try again a valid one. Valid: list_hunts, list_treasures <hunt_id>, view_treasure <hunt_id> <treasure_id>, exit\n");
        }

        // verifică dacă monitorul a murit
        int status;
        if (waitpid(pid, &status, WNOHANG) == pid) { // comanda WNOHANG verifica daca procesul copil a terminat, dar nu aasteapta daca nu s a intamplat inca
            printf("[INFO] Monitor has exited.\n");
            break;
        }
    }

    return 0;
}
