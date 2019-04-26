#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <libgen.h>
#include <sys/time.h>

#define PROCESSES_COUNT 9
#define PARENTS_COUNT 5

char *MODULE_NAME;
int *PROCESSES_PIDS;

typedef struct {
    int process_number;
    int children_count;
    int *children;
} forking_info_t;

int PROCESS_NUMBER = 0;
int sig_received = 0;
int sig_usr1_sent = 0;
int sig_usr2_sent = 0;

int RECEIVERS_IDS[PROCESSES_COUNT] = {-1, 2, -1, -1, -1, 6, -1, -1, 1};
int SIGNALS_TO_SEND[PROCESSES_COUNT] = {-1, SIGUSR1, -1, -1, -1, SIGUSR1, -1, -1, SIGUSR1};
int GROUPS_INFO[PROCESSES_COUNT] = {0, 1, 2, 2, 2, 2, 6, 6, 6};

forking_info_t *FORKING_SCHEME;

void printError(const char *module_name, const char *error_msg, const char *function_name) {
    fprintf(stderr, "%s: %s: %s\n", module_name, function_name ? function_name : "", error_msg);
}

long long currentTime() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return time.tv_usec / 1000;
}

char *signalName(int signum) {
    switch (signum) {
        case SIGUSR1:
            return "USR1";
        case SIGUSR2:
            return "USR2";
        default:
            return "not found";
    }
}

void printInfo(int process_number, char is_received, int signal_number) {
    printf("%d %d %d %s %s %lld\n", process_number, getpid(), getppid(), is_received ? "получил" : "послал",
           signalName(signal_number), currentTime());
    fflush(stdout);
}

void printStat(int usr1_count, int usr2_count) {
    printf("%d %d завершил работу после %d-го сигнала SIGUSR1 и %d-го сигнала SIGUSR2\n", getpid(), getppid(), usr1_count,
           usr2_count);
}

void createForkingScheme(forking_info_t *forking_scheme) {

    forking_scheme[0].process_number = 0;
    forking_scheme[0].children_count = 1;
    forking_scheme[0].children = malloc(sizeof(int));
    forking_scheme[0].children[0] = 1;

    forking_scheme[1].process_number = 1;
    forking_scheme[1].children_count = 4;
    forking_scheme[1].children = malloc(sizeof(int) * 4);
    forking_scheme[1].children[0] = 2;
    forking_scheme[1].children[1] = 3;
    forking_scheme[1].children[2] = 4;
    forking_scheme[1].children[3] = 5;

    forking_scheme[2].process_number = 2;
    forking_scheme[2].children_count = 1;
    forking_scheme[2].children = malloc(sizeof(int));
    forking_scheme[2].children[0] = 6;

    forking_scheme[3].process_number = 3;
    forking_scheme[3].children_count = 1;
    forking_scheme[3].children = malloc(sizeof(int));
    forking_scheme[3].children[0] = 7;

    forking_scheme[4].process_number = 4;
    forking_scheme[4].children_count = 1;
    forking_scheme[4].children = malloc(sizeof(int));
    forking_scheme[4].children[0] = 8;
}


forking_info_t *getForkInfo(int process_number, forking_info_t *forking_scheme) {
    for (int i = 0; i < PARENTS_COUNT; i++) {
        if (forking_scheme[i].process_number == process_number) {
            return &(forking_scheme[i]);
        }
    }
    return NULL;
}

void signalHandler(int signum) {
    if (signum != SIGTERM) {
        printInfo(PROCESS_NUMBER, 1, signum);
        int receiver_number = RECEIVERS_IDS[PROCESS_NUMBER];
        sig_received++;
        if (PROCESS_NUMBER == 1) {
            if (sig_received == 101) {
                if (kill(-PROCESSES_PIDS[RECEIVERS_IDS[PROCESS_NUMBER]], SIGTERM) == -1) {
                    printError(MODULE_NAME, strerror(errno), "kill");
                    exit(0);
                };
                while (wait(NULL) > 0);
                exit(0);
            }
        }
        if (receiver_number != -1) {
            int signal_to_send = SIGNALS_TO_SEND[PROCESS_NUMBER];
            usleep(100);
            if (kill(-PROCESSES_PIDS[RECEIVERS_IDS[PROCESS_NUMBER]], signal_to_send) == -1) {
                printError(MODULE_NAME, strerror(errno), "kill");
                exit(1);
            } else {
                printInfo(PROCESS_NUMBER, 0, signal_to_send);
                (signal_to_send == SIGUSR1) ? sig_usr1_sent++ : sig_usr2_sent++;
            };
        }
    } else {
        if (kill(-PROCESSES_PIDS[RECEIVERS_IDS[PROCESS_NUMBER]], SIGTERM) == -1) {
            printError(MODULE_NAME, strerror(errno), "kill");
            exit(0);
        };
        while (wait(NULL) > 0);
        if (PROCESS_NUMBER != 1) {
            printStat(sig_usr1_sent, sig_usr2_sent);
        }
        exit(0);
    }
}

void clearForkingScheme(forking_info_t *forking_scheme) {
    for (int i = 0; i < PARENTS_COUNT; i++) {
        free(forking_scheme[i].children);
    }
    free(forking_scheme);
}

char isAllSet(pid_t *process_pids) {
    for (int i = 0; i < PROCESSES_COUNT; i++) {
        if (!process_pids[i]) {
            return 0;
        }
    }
    return 1;
}

void startProcess() {
    PROCESSES_PIDS[0] = getpid();
    int forked_process_number;
    int child_number = 0;
    pid_t group_leader;
    forking_info_t *forking_info;

    struct sigaction sig_handler;
    sig_handler.sa_handler = &signalHandler;
    sig_handler.sa_flags = 0;
    sigset_t sigset;
    sigemptyset(&sigset);

    while ((forking_info = getForkInfo(PROCESS_NUMBER, FORKING_SCHEME)) &&
           (child_number < forking_info->children_count)) {
        forked_process_number = forking_info->children[child_number];
        pid_t child = fork();
        switch (child) {
            case 0:
                child_number = 0;
                PROCESS_NUMBER = forked_process_number;
                break;
            case -1:
                printError(MODULE_NAME, strerror(errno), "fork");
                exit(1);
            default:
                while (!(group_leader = PROCESSES_PIDS[GROUPS_INFO[forked_process_number]]));
                if (setpgid(child, group_leader) == -1) {
                    printError(MODULE_NAME, strerror(errno), "setpgid");
                    exit(1);
                };
                child_number++;
        }
    }
    if (sigaction(SIGUSR1, &sig_handler, 0) == -1) {
        printError(MODULE_NAME, strerror(errno), "sigaction");
    };
    if (sigaction(SIGUSR2, &sig_handler, 0) == -1) {
        printError(MODULE_NAME, strerror(errno), "sigaction");
    };
    if (sigaction(SIGTERM, &sig_handler, 0) == -1) {
        printError(MODULE_NAME, strerror(errno), "sigaction");
    };

    PROCESSES_PIDS[PROCESS_NUMBER] = getpid();
    if (PROCESS_NUMBER == 1) {
        while (!isAllSet(PROCESSES_PIDS));
        int signal_to_send = SIGNALS_TO_SEND[PROCESS_NUMBER];
        kill(-PROCESSES_PIDS[RECEIVERS_IDS[PROCESS_NUMBER]], signal_to_send);
        printInfo(PROCESS_NUMBER, 0, signal_to_send);
    }
    if (PROCESS_NUMBER == 0) {
        wait(NULL);
        return;
    }
    while (1){
        sleep(1);
    };
}


int main(int argc, char *argv[]) {
    MODULE_NAME = basename(argv[0]);
    FORKING_SCHEME = malloc(sizeof(forking_info_t) * PARENTS_COUNT);

    createForkingScheme(FORKING_SCHEME);
    if (!(PROCESSES_PIDS = mmap(NULL, PROCESSES_COUNT * sizeof(pid_t), PROT_READ | PROT_WRITE,
                                MAP_SHARED | MAP_ANONYMOUS, -1, 0))) {
        printError(MODULE_NAME, strerror(errno), "mmap");
        return 1;
    }

    startProcess();

    if (munmap(PROCESSES_PIDS, sizeof(pid_t) * PROCESSES_COUNT) == -1) {
        printError(MODULE_NAME, strerror(errno), "munmap");
    };
    clearForkingScheme(FORKING_SCHEME);
    return 0;
}
