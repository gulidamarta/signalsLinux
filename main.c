#include <stdbool.h>
#include <libgen.h>
#include <stdio.h>
#include <sys/stat.h>
#include <memory.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <semaphore.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>

#define ARGUMENTS_AMOUNT 2
#define LEN_PATH 256

char *module_name;
FILE *out;
static pid_t group_id, parent, base_group, second_proc;
static int received = 0;

void printError(const char *module_name, const char *error_msg, const char *file_name) {
    fprintf(stderr, "%s: %s %s\n", module_name, error_msg, file_name ? file_name : "");
}


void putFile(const char *num) {
    char *name = (char *)malloc(sizeof(char) * LEN_PATH);
    if (name) {
        strcpy(name, "/tmp/LAB4/");
        strcat(name, num);
        strcat(name, ".pid\0");
        FILE *f = fopen(name, "w+");
        if (f) {
            //fprintf(f, "%d", getpid());
            fclose(f);
        } else {
            printError(module_name, "File can not be open.", name);
        }
        free(name);
    } else {
        printError(module_name, "Memory allocation error.", NULL);
    }
}

bool isForkingDone() {
    int count = 0;
    DIR *dir;
    if ((dir = opendir("/tmp/LAB4"))) {
        struct dirent *direntbuf;
        while ((direntbuf = readdir(dir))) {
            count += direntbuf->d_type == DT_REG && !strcmp(direntbuf->d_name + 1, ".pid");
        }
        closedir(dir);
    }
    return count == 8;
}

long long getTime() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_usec % 1000;
}

void process1SIGUSR() {
    //usleep(880);
    if (received == 101) {

        printf("1 %d %d send SIGTERM %lld\n", getpid(), getppid(), getTime());
        fprintf(out, "1 %d %d send SIGTERM %lld\n", getpid(), getppid(), getTime());

        kill(-base_group, SIGTERM);

        while (wait(NULL) != -1);
        //wait for children
        printf("1 %d %d %d SIGUSR1 end\n", getpid(), getppid(), received);
        fprintf(out, "1 %d %d %d SIGUSR1 end\n", getpid(), getppid(), received);

        exit(0);
    } else {
        //sleep(1);
        printf("1 %d %d got SIGUSR1 %lld\n", getpid(), getppid(), getTime());
        fprintf(out, "1 %d %d got SIGUSR1 %lld\n", getpid(), getppid(), getTime());

        received++;

        printf("1 %d %d send SIGUSR1 %lld\n", getpid(), getppid(), getTime());
        fprintf(out, "1 %d %d send SIGUSR1 %lld\n", getpid(), getppid(), getTime());

        kill(-base_group, SIGUSR1);
    }
}

void process2SIGUSR() {
    printf("2 %d %d got SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    fprintf(out, "2 %d %d got SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    received++;
}

void process3SIGUSR() {
    printf("3 %d %d got SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    fprintf(out, "3 %d %d got SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    received++;
}

void process4SIGUSR() {
    printf("4 %d %d got SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    fprintf(out, "4 %d %d got SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    received++;
}

void process5SIGUSR() {
    printf("5 %d %d got SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    fprintf(out, "5 %d %d got SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    received++;
    //usleep(440);
    printf("5 %d %d send SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    fprintf(out, "5 %d %d send SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    kill(-group_id, SIGUSR1);
}

void process6SIGUSR() {
    printf("6 %d %d got SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    fprintf(out, "6 %d %d got SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    received++;
}

void process7SIGUSR() {
    printf("7 %d %d got SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    fprintf(out, "7 %d %d SIGUSR1 got %lld\n", getpid(), getppid(), getTime());
    received++;
}

void process8SIGUSR() {
    printf("8 %d %d got SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    fprintf(out, "8 %d %d got SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    received++;
    //usleep(440);
    printf("8 %d %d send SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    fprintf(out, "8 %d %d send SIGUSR1 %lld\n", getpid(), getppid(), getTime());
    kill(parent, SIGUSR1);
}

void process2SIGTERM() {
    printf("2 %d %d %d SIGUSR1 end %d\n", getpid(), getppid(), received, getpgid(getpid()));
    fprintf(out, "2 %d %d %d SIGUSR1 end\n", getpid(), getppid(), received);
    exit(0);
}

void process3SIGTERM() {
    printf("3 %d %d %d SIGUSR1 end %d\n", getpid(), getppid(), received, getpgid(getpid()));
    fprintf(out, "3 %d %d %d SIGUSR1 end\n", getpid(), getppid(), received);
    exit(0);
}

void process4SIGTERM() {
    printf("4 %d %d %d SIGUSR1 end %d\n", getpid(), getppid(), received, getpgid(getpid()));
    fprintf(out, "4 %d %d %d SIGUSR1 end\n", getpid(), getppid(), received);
    exit(0);
}

void process5SIGTERM() {
    printf("5 %d %d %d send SIGTERM\n", getpid(), getppid(), received);
    fprintf(out, "5 %d %d %d send SIGTERM\n", getpid(), getppid(), received);
    kill(-group_id, SIGTERM);
    while (wait(NULL) != -1);
    //wait for children
    printf("5 %d %d %d SIGUSR1 end %d\n", getpid(), getppid(), received, getpgid(getpid()));
    fprintf(out, "5 %d %d %d SIGUSR1 end\n", getpid(), getppid(), received);
    exit(0);
}

void process6SIGTERM() {
    printf("6 %d %d %d SIGUSR1 end %d\n", getpid(), getppid(), received, getpgid(getpid()));
    fprintf(out, "6 %d %d %d SIGUSR1 end\n", getpid(), getppid(), received);
    exit(0);
}

void process7SIGTERM() {
    printf("7 %d %d %d SIGUSR1 end %d\n", getpid(), getppid(), received, getpgid(getpid()));
    fprintf(out, "7 %d %d %d SIGUSR1 end\n", getpid(), getppid(), received);
    exit(0);
}

void process8SIGTERM() {
    printf("8 %d %d %d SIGUSR1 end %d\n", getpid(), getppid(), received, getpgid(getpid()));
    fprintf(out, "8 %d %d %d SIGUSR1 end\n", getpid(), getppid(), received);
    exit(0);
}


void startProcesses() {
    parent = fork();
    if (parent == -1) {
        printError(module_name, "Unable to create process.", NULL);
        return;
    }

    if (parent > 0) {
        while (wait(NULL) != -1);
        return;
    }

    if (!parent) {

        struct sigaction handler;
        handler.sa_handler = &process1SIGUSR;
        sigaction(SIGUSR1, &handler, 0);
        sigfillset(&handler.sa_mask);
        handler.sa_flags = SA_RESTART;

        parent = getpid();
        putFile("1");

        pid_t child;
        child = fork();
        second_proc = child;
        if (!child) {
            //2
            handler.sa_handler = &process2SIGUSR;
            sigaction(SIGUSR1, &handler, 0);
            handler.sa_handler = &process2SIGTERM;
            sigaction(SIGTERM, &handler, 0);
            //setpgid(getpid(), getpid());
            putFile("2");
            while (1);
        } else if (child > 0) {
            setpgid(child, child);
            group_id = child;
            base_group = child;
            child = fork();
            if (!child) {
                //3
                handler.sa_handler = &process3SIGUSR;
                sigaction(SIGUSR1, &handler, 0);
                handler.sa_handler = &process3SIGTERM;
                sigaction(SIGTERM, &handler, 0);
                //setpgid(getpid(), group_id);
                putFile("3");
                while (1);
            } else if (child > 0) {
                setpgid(child, group_id);
                child = fork();
                if (!child) {
                    //4
                    handler.sa_handler = &process4SIGUSR;
                    sigaction(SIGUSR1, &handler, 0);
                    handler.sa_handler = &process4SIGTERM;
                    sigaction(SIGTERM, &handler, 0);
                    //setpgid(getpid(), group_id);
                    putFile("4");
                    while (1);
                } else if (child > 0) {
                    setpgid(child, group_id);
                    child = fork();
                    if (!child) {
                        //5
                        handler.sa_handler = &process5SIGUSR;
                        sigaction(SIGUSR1, &handler, 0);
                        handler.sa_handler = &process5SIGTERM;
                        sigaction(SIGTERM, &handler, 0);
                        //setpgid(getpid(), group_id);
                        putFile("5");
                        child = fork();
                        if (!child) {
                            //6
                            handler.sa_handler = &process6SIGUSR;
                            sigaction(SIGUSR1, &handler, 0);
                            handler.sa_handler = &process6SIGTERM;
                            sigaction(SIGTERM, &handler, 0);
                            //setpgid(getpid(), getpid());
                            putFile("6");
                            while (1);
                        } else if (child > 0) {
                            setpgid(child, child);
                            group_id = child;
                            child = fork();
                            if (!child) {
                                //7
                                handler.sa_handler = &process7SIGUSR;
                                sigaction(SIGUSR1, &handler, 0);
                                handler.sa_handler = &process7SIGTERM;
                                sigaction(SIGTERM, &handler, 0);
                                //setpgid(getpid(), group_id);
                                putFile("7");
                                while (1);
                            } else if (child > 0) {
                                setpgid(child, group_id);
                                child = fork();
                                if (!child) {
                                    //8
                                    handler.sa_handler = &process8SIGUSR;
                                    sigaction(SIGUSR1, &handler, 0);
                                    handler.sa_handler = &process8SIGTERM;
                                    sigaction(SIGTERM, &handler, 0);
                                    //setpgid(getpid(), group_id);
                                    putFile("8");
                                    while (1);
                                } else if (child > 0) {
                                    setpgid(child, group_id);
                                }
                            }
                        }
                        while (1);
                    } else
                        if (child > 0) {
                            setpgid(child, group_id);
                    }
                }
            }
        }
        while (!isForkingDone());

        kill(-base_group, SIGUSR1);

        while (1);
    }
}


int main(int argc, char *argv[]) {
    module_name = basename(argv[0]);
    if (argc < ARGUMENTS_AMOUNT){
        printError(module_name, "The amount of arguments is not correct.", NULL);
        return 1;
    }

    if (!(out = fopen(argv[1], "w+"))) {
        printError(module_name, "File cannot be open.", argv[1]);
        return 1;
    }

    if (mkdir("/tmp/LAB4", 0777)) {
        printError(module_name, "Unable to create directory.", "tmp/LAB4");
        return 1;
    }

    startProcesses();
    system("rm -r /tmp/LAB4");
    fclose(out);
    //while (wait(NULL) != -1);
    return 0;
}

