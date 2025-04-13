#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#define MAX_ID_LEN 32
#define MAX_USERNAME_LEN 32
#define MAX_CLUE_LEN 128

typedef struct {
    char id[MAX_ID_LEN];
    char username[MAX_USERNAME_LEN];
    float latitude;
    float longitude;
    char clue[MAX_CLUE_LEN];
    int value;
} Treasure; //structura unei comori

//funcție pentru citirea unei linii de la tastatură 
ssize_t safe_read_line(char *buffer, size_t size) {
    ssize_t len = read(0, buffer, size - 1);
    if (len > 0) {
        if (buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        } else {
            buffer[len] = '\0';
        }
    } else {
        buffer[0] = '\0';
    }
    return len;
}

void print_usage() {
    write(1, "Usage:\n", 7);
    write(1, "  --add <hunt_id>\n", 19);
    write(1, "  --list <hunt_id>\n", 20);
    write(1, "  --view <hunt_id> <treasure_id>\n", 33);
    write(1, "  --remove_treasure <hunt_id> <treasure_id>\n", 44);
    write(1, "  --remove_hunt <hunt_id>\n", 27);
} //mesajul de eroare
int id_exists(char *hunt_id, char *id) {//functie pt a avea id ul unic
    char file_path[512]; 
    snprintf(file_path, sizeof(file_path), "hunts/%s/treasures.dat", hunt_id);
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) return 0; //fișierul nu există, deci sigur nu există id ul
    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.id, id) == 0) {
            close(fd);
            return 1; //găsit
        }
    }

    close(fd);
    return 0; //nu s a găsit
}
void add_treasure(char *hunt_id) {
    char dir_path[512];
    snprintf(dir_path, sizeof(dir_path), "hunts/%s", hunt_id);

    //creează directorul principal și subdirectorul dacă nu există
    mkdir("hunts", 0777);
    if (mkdir(dir_path, 0777) == -1 && errno != EEXIST) {
        perror("error mkdir");
        return;
    }

    char treasure_file[1024];
    snprintf(treasure_file, sizeof(treasure_file), "%s/treasures.dat", dir_path);
    int fd = open(treasure_file, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (fd == -1) {
        perror("open treasure file");
        return;
    }

    Treasure t;
    char buffer[256];

    do {
        write(1, "Enter treasure ID: ", 20);
        safe_read_line(t.id, MAX_ID_LEN);
    
        if (id_exists(hunt_id, t.id)) {
            char *msg="ID already exists. Enter a unique ID.\n";
            write(1,msg , strlen(msg));
        }
    } while (id_exists(hunt_id, t.id));
    

    write(1, "Enter username: ", 17);
    safe_read_line(t.username, MAX_USERNAME_LEN);

    write(1, "Enter latitude: ", 17);
    safe_read_line(buffer, sizeof(buffer));
    t.latitude = strtof(buffer, NULL);

    write(1, "Enter longitude: ", 18);
    safe_read_line(buffer, sizeof(buffer));
    t.longitude = strtof(buffer, NULL);

    write(1, "Enter clue: ", 12);
    safe_read_line(t.clue, MAX_CLUE_LEN);

    write(1, "Enter value: ", 13);
    safe_read_line(buffer, sizeof(buffer));
    t.value = atoi(buffer);

    if (write(fd, &t, sizeof(Treasure)) == -1) {
        perror("write");
        close(fd);
        return;
    }

    close(fd);

    // scriere in fisierul text
    char log_path[1024];
    int n=snprintf(log_path, sizeof(log_path), "%s/logged_hunt", dir_path);
    if(n<0 || n>=sizeof(log_path)){
        write(2,"Error: path too long for logged_hunt",37);
        exit(-1);
    }
    int log_fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (log_fd != -1) {
        time_t now = time(NULL);
        char *time_str = ctime(&now);
        time_str[strcspn(time_str, "\n")] = 0;
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "[%s] Added treasure %s by %s\n", time_str, t.id, t.username);
        write(log_fd, log_msg, strlen(log_msg));
        close(log_fd);
    }

    char symlink_name[512];
    snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
    symlink(log_path, symlink_name);

    write(1, "Treasure added successfully!\n", 30);
}

void list_treasures(const char *hunt_id) {//afiseaza info despre fisierul cu comori si le citeste una cate una
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "hunts/%s/treasures.dat", hunt_id);

    struct stat st;
    if (stat(file_path, &st) == -1) {//obtine dimensiunea si data ultimei modificari
        perror("error stat");
        exit(-1);
    }

    char header[512];
    snprintf(header, sizeof(header), "Hunt: %s\nFile size: %ld bytes\nLast modified: %s\n",
             hunt_id, (long)st.st_size, ctime(&st.st_mtime));
    write(1, header, strlen(header));

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("error open");
        exit(-1);
    }

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {//parcurge toate comorile din fisier si le afiseaza
        char info[512];
        snprintf(info, sizeof(info),
                 "ID: %s | User: %s | Lat: %f | Long: %f | Value: %d\n  Clue: %s\n",
                 t.id, t.username, t.latitude, t.longitude, t.value, t.clue);
        write(1, info, strlen(info));
    }

    close(fd);
}

void view_treasure(char *hunt_id, char *treasure_id) {//cauta o comoara dupa id si o afiseaza
    char file_path[512];
    snprintf(file_path, sizeof(file_path), "hunts/%s/treasures.dat", hunt_id);
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("error open");
        exit(-1);
    }

    Treasure t;
    int found = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.id, treasure_id) == 0) {//compara id ul cautat cu cel din structura
            found = 1;
            char details[512];
            snprintf(details, sizeof(details),
                     "Treasure ID: %s\nUsername: %s\nLatitude: %f\nLongitude: %f\nClue: %s\nValue: %d\n",
                     t.id, t.username, t.latitude, t.longitude, t.clue, t.value);
            write(1, details, strlen(details));
            break;
        }
    }

    close(fd);

    if (!found) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Treasure with ID '%s' not found in hunt '%s'.\n", treasure_id, hunt_id);
        write(1, msg, strlen(msg));
    }
}
void remove_treasure(char *hunt_id, char *treasure_id) {//functia pt a elimina o comoara
    char file_path[512], temp_path[512];//creeaza un fisier temporar fara comoara care tebuie stearsa
    snprintf(file_path, sizeof(file_path), "hunts/%s/treasures.dat", hunt_id);
    snprintf(temp_path, sizeof(temp_path), "hunts/%s/temp.dat", hunt_id);

    int fd_old = open(file_path, O_RDONLY);
    int fd_new = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd_old == -1 || fd_new == -1) {
        perror("error open");
        exit(-1);
    }
    Treasure t;
    int found = 0;
    while (read(fd_old, &t, sizeof(Treasure)) == sizeof(Treasure)) {
        if (strcmp(t.id, treasure_id) == 0) {//sare peste comoara care trebuie stearsa
            found = 1;
            continue;
        }
        write(fd_new, &t, sizeof(Treasure));
    }

    close(fd_old);
    close(fd_new);

    if (found) {
        remove(file_path);
        rename(temp_path, file_path); //inlocuieste fisierul original cu cel temporar(fara comoara)

        char log_path[512];
        int n=snprintf(log_path, sizeof(log_path), "hunts/%s/logged_hunt", hunt_id);
        if(n<0 || n>=sizeof(log_path)){
            write(2,"Error: path too long for logged_hunt",37);
            exit(-1);
        }
        int log_fd = open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
        if (log_fd != -1) {
            time_t now = time(NULL);
            char *time_str = ctime(&now);
            time_str[strcspn(time_str, "\n")] = 0;
            char log_msg[256];
            snprintf(log_msg, sizeof(log_msg), "[%s] Removed treasure %s\n", time_str, treasure_id);
            write(log_fd, log_msg, strlen(log_msg));
            close(log_fd);
        }

        char msg[256];
        snprintf(msg, sizeof(msg), "Treasure %s removed successfully from hunt %s.\n", treasure_id, hunt_id);
        write(1, msg, strlen(msg));
    } else {
        remove(temp_path);
        char msg[256];
        snprintf(msg, sizeof(msg), "Treasure %s not found in hunt %s.\n", treasure_id, hunt_id);
        write(1, msg, strlen(msg));
    }
}

void remove_hunt(char *hunt_id) {//sterge tot ce apartine unei vanatori
    char dir_path[256], treasure_file[256], log_file[256], symlink_name[256];
    int n=snprintf(dir_path, sizeof(dir_path), "hunts/%s", hunt_id);
    if(n<0 || n>=sizeof(log_file)){
        write(2,"Error: path too long for hunts",31);
        exit(-1);
    }
    int m=snprintf(treasure_file, sizeof(treasure_file), "%s/treasures.dat", dir_path);
    if(m<0 || m>=sizeof(log_file)){
        write(2,"Error: path too long for treasures.dat",37);
        exit(-1);
    }
    int s=snprintf(log_file, sizeof(log_file), "%s/logged_hunt", dir_path);
    if(s<0 || s>=sizeof(log_file)){
        write(2,"Error: path too long for logged_hunt",37);
        exit(-1);
    }
    int d=snprintf(symlink_name, sizeof(symlink_name), "logged_hunt-%s", hunt_id);
    if(d<0 || d>=sizeof(log_file)){
        write(2,"Error: path too long for logged_hunt",37);
        exit(-1);
    }
    int c = 0;
//sterge fisierele,directorul si symlink ul
    if (unlink(treasure_file) == -1 && errno != ENOENT) {
        perror("unlink treasures.dat");
        c++;
    }

    if (unlink(log_file) == -1 && errno != ENOENT) {
        perror("unlink logged_hunt");
        c++;
    }

    if (rmdir(dir_path) == -1) {
        perror("rmdir hunt folder");
        c++;
    }

    if (unlink(symlink_name) == -1 && errno != ENOENT) {
        perror("unlink symlink");
        c++;
    }

    if (c == 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Hunt %s removed successfully.\n", hunt_id);
        write(1, msg, strlen(msg));
    } else {
        write(1, "Some files may not have been deleted properly.\n", 47);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage();
        return 1;
    }

    char *command = argv[1];
    char *hunt_id = argv[2];

    if (strcmp(command, "--add") == 0) {
        char a[8];
        do {
            add_treasure(hunt_id);
            write(1, "Add another treasure? y/n\n", 27);
            safe_read_line(a, sizeof(a));
        } while (a[0] == 'y');
    } else if (strcmp(command, "--list") == 0) {
        list_treasures(hunt_id);
    } else if (strcmp(command, "--view") == 0 && argc == 4) {
        view_treasure(hunt_id, argv[3]);
    } else if (strcmp(command, "--remove_treasure") == 0 && argc == 4) {
        remove_treasure(hunt_id, argv[3]);
    } else if (strcmp(command, "--remove_hunt") == 0) {
        remove_hunt(hunt_id);
    } else {
        print_usage();
        return 1;
    }

    return 0;
}