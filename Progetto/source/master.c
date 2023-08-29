#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/fcntl.h>
#include "utils.h"
#include "stringlist.h"
#include "syncqueue.h"
#include "farm.h"

static int connect_to_collector();
int get_options(int argc, char *argv[], stringlist argfiles, bool *d_isset, char *d_arg,
                bool *n_isset, int *n_arg, bool *q_isset, int *q_arg, bool *t_isset, int *t_arg);
int explore_dir(stringlist argfiles, char *full_path, char *dir);
void install_handlers();
static void *worker_task(void *arg);
static void delay_insert(int delay);

//Flags set by signals
volatile sig_atomic_t interrupted;
volatile sig_atomic_t interrupted_usr1;

int main(int argc, char *argv[]){
    sigmask_all();
    install_handlers();
    if(argc < 2){
        PRINT_USAGE;
        return -1;
    }
    if(master(argc, argv) == -1){
        print_error(errno, NULL);
        return -1;
    }
    return 0;
}

int master(int argc, char* argv[]){
    int ret;
    interrupted = 0;
    interrupted_usr1 = 0;
    stringlist argfiles;
    stringlist_init(&argfiles, MAX_PATH_LEN);

    bool n_isset = false, q_isset = false, t_isset = false, d_isset = false;
    int narg = 0, qlen = 0, delay = 0;
    char d_arg[MAX_PATH_LEN + 1];
    ret = get_options(argc, argv, argfiles, &d_isset, d_arg, &n_isset, &narg, &q_isset, &qlen, &t_isset, &delay);
    if(d_isset){
        ret = explore_dir(argfiles, d_arg, d_arg);
    }
//    char *pathh;
//    while(stringlist_get(argfiles, pathh) != -1){
//        printf("%s\n", pathh);
//    }
    int collector_pid;
    //Will hold messages from master to collector
    char msg;
    int nthreads = n_isset ? narg : THREADS_DEFAULT_N;
    //Will hold exit codes of worker threads
    int w_term_statuses[nthreads];
    //Socket to communicate with collector process
    int sck_collector = -1;
    int pool_is_init = false;

    sync_queue queue;
    if(sync_queue_init(&queue, MAX_PATH_LEN , q_isset ? qlen : QUEUE_DEFAULT_SZ) == -1){
        goto cleanup;
    }
    if(ret == -1){
        goto cleanup;
    }
    //Creates the collector process
    switch(collector_pid = fork()){
        case -1:
            goto cleanup;
        case 0: {
            execl("collector", NULL);
                printf("Failed\n");
                return -1;
        }
        default:
            break;
    }
    sck_collector = connect_to_collector();
    if (sck_collector == -1){
        goto cleanup;
    }

    //Sets connection type with collector to "master"
    char type_msg = TYPE_MASTER;
    if(writen(sck_collector, &type_msg, TYPE_SZ) != TYPE_SZ){
                goto cleanup;
            }
    //Creates the worker pool, passing "queue" as argument to the threads
    worker_pool pool;
    pool_is_init = true;
    if (worker_pool_init(&pool, worker_task, nthreads, queue) != nthreads){
        goto cleanup;
    }
    //Manage signals and adds namefiles to "queue"
    char next_file[256];
    sigunmask_all();
    while(!interrupted && stringlist_get(argfiles, next_file) == 0){
        if(interrupted_usr1){
            interrupted_usr1 = 0;
            msg = PRINT;
            if(writen(sck_collector, &msg, MASTER_MSG_SZ) != MASTER_MSG_SZ){
                if(errno == EINTR) continue;
                print_error(errno, NULL);
                goto cleanup;
            }
            continue;
        }
        sync_queue_put(queue, next_file);
//        printf("Master queued:\n%s\n", next_file);
        //If "t" option is set, delays insertion of next element by "delay" milliseconds
        if(t_isset){
           delay_insert(delay);
        }
    }
    sigmask_all();
    await_queue_empty(queue);
    ret = 0;

    cleanup:
    terminate(queue);
    if(pool_is_init){
        if(worker_pool_destroy(pool, w_term_statuses) == -1) ret = -1;
    }
    for(int i = 0; i < nthreads; i++){
        if (w_term_statuses[i] != 0) ret = -1;
    }
    msg = QUIT;
    if(writen(sck_collector, &msg , MASTER_MSG_SZ) != MASTER_MSG_SZ){
        //If no clean termination is possible
        kill(collector_pid, SIGKILL);
        ret = -1;
    }
    int c_term_status;
    waitpid(collector_pid, &c_term_status, 0);
    if(!WIFEXITED(c_term_status) || WEXITSTATUS(c_term_status) != 0){
        ret = -1;
    }
    sync_queue_destroy(queue);
    stringlist_destroy(argfiles);
    unlink("farm.sck");
    return ret;
}

static void *worker_task(void *arg){
//    fprintf(stderr, "Worker in\n");
    int ret = -1;
    FILE *currfile = NULL;
    sync_queue file_queue = (sync_queue) arg;
    char filepath[MAX_PATH_LEN + 1];
    int fd_sock = connect_to_collector();
    if(fd_sock == -1){
        goto cleanup;
    }
    char type_msg = TYPE_WORKER;
    if(write(fd_sock, &type_msg, TYPE_SZ) != TYPE_SZ){
        goto cleanup;
    }
    while(sync_queue_get(file_queue, filepath) == 0){
//        printf("Worker dequed:\n%s\n", filepath);
        if((currfile = fopen(filepath, "r")) == NULL){
            goto cleanup;
        }
        long currnum;
        long sum = 0;
        int i = 0;

        while(fread(&currnum, sizeof(long), 1, currfile) != 0){
            sum += currnum * i;
            i++;
        }
        if(!feof(currfile)){
            goto cleanup;
        }
        char msg[WORKER_MSG_SZ];
        //Writes sum in the first sizeof(long) bytes of msg
        long *sum_location = (long*)msg;
        *sum_location = sum;
        //Writes filepath to msg starting from the sizeof(sum) index of the msg buffer
        strlcpy(msg + sizeof(sum), filepath, WORKER_MSG_SZ - sizeof(sum));
        if(writen(fd_sock, msg, WORKER_MSG_SZ) != WORKER_MSG_SZ){
            goto cleanup;
        }
        if(fclose(currfile) != 0){
            currfile = NULL;
            goto cleanup;
        }
        currfile = NULL;
    }
    ret = 0;

    cleanup:
    if(currfile != NULL){
        if(fclose(currfile) != 0)
            ret = -1;
    }
    if(close(fd_sock) != 0){
        ret = -1;
    }
//    fprintf(stderr, "Worker out\n");

    return (void*)ret;
}

static int connect_to_collector(){
    int fd_sock;
    int res = -1;
    struct sockaddr_un sa;
    strlcpy(sa.sun_path, "farm.sck", sizeof(sa.sun_path));
    sa.sun_family = AF_UNIX;

    if((fd_sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        goto fail;
    }
    for(int i = 0; i < 20; i++){
        errno = 0;
        struct timespec time;
        time.tv_sec = 0;
        time.tv_nsec = 20000000; //20ms

        if((res = connect(fd_sock,(struct sockaddr*)&sa, sizeof(sa))) == -1){
            if(errno == ENOENT){  //DA CONTROLLARE
                nanosleep(&time, NULL);
                time.tv_nsec *= 2;
            }
            else goto fail;
        }
        else break;
    }
    goto ret;

    fail:
    if(fd_sock != -1){
        close(fd_sock);
    }

    ret:
    if(res != -1)
        return fd_sock;
    else
        return -1;
}

static void delay_insert(int delay){
    struct timespec rqtp;
    struct timespec rmtp;
    rqtp.tv_sec = delay / 1000;
    rqtp.tv_nsec = (delay % 1000) * 1,000,000;
    while(nanosleep(&rqtp, &rmtp) == -1){
        rqtp.tv_sec = rmtp.tv_sec;
        rqtp.tv_nsec = rmtp.tv_nsec;
    }
}

int get_options(int argc, char *argv[], stringlist argfiles, bool *d_isset, char *d_arg,
                bool *n_isset, int *n_arg, bool *q_isset, int *q_arg, bool *t_isset, int *t_arg){
    char optstring[] = ":n:q:t:d:";
    int ch;

    //Check for options in argv
    while (optind < argc) {
        ch = getopt(argc, argv, optstring);
        char *endptr;
        switch (ch) {
            case 'h':
            PRINT_USAGE;
                goto fail;
            case 'n':
                *n_isset = true;
                *n_arg = (int) strtol(optarg, &endptr, 0);
                if (*n_arg < 0) {
                    PRINT_USAGE;
                    goto fail;
                }
                break;
            case 'q':
                *q_isset = true;
                *q_arg = (int) strtol(optarg, &endptr, 0);
                if (*q_arg < 0) {
                    PRINT_USAGE;
                    goto fail;
                }
                break;
            case 't':
                *t_isset = true;
                *t_arg = (int) strtol(optarg, &endptr, 0);
                if (*t_arg < 0) {
                    PRINT_USAGE;
                    goto fail;
                }
                break;
            case 'd':
                *d_isset = true;
                strlcpy(d_arg, optarg, MAX_PATH_LEN + 1);
                break;
            case ':':
            PRINT_USAGE;
                goto fail;
            default: {
                //Filter filenames from options and save them in argfiles
                struct stat info;
                if (stat(argv[optind], &info) != 0) {
                    print_error(errno, NULL);
                    goto fail;
                }
                if (!S_ISREG(info.st_mode)) {
                    print_error(EFTYPE, argv[optind]);
                    goto fail;
                }
                if (stringlist_put(argfiles, argv[optind])) {
                    print_error(errno, argv[optind]);
                }
                optind++;
                break;
            }
        }
    }
    return 0;

    fail:
    return -1;
}

//Explores the current directory recursively and adds valid pathnames to argfiles. Stops and returns -1 when it finds a path
//that is not a directory nor a regular file, otherwise returns 0.
//full_path holds the path from the starting directory till the current one
int explore_dir(stringlist argfiles, char *full_path, char *dir){
    //Pointers to resources to be later freed
    int ret = -1;
    int owd;
    DIR *currdir = NULL;

    char path[MAX_PATH_LEN + 1];

    if((owd = open(".", O_RDONLY)) == -1){
        print_error(errno, NULL);
        goto cleanup;
    }
    if (chdir(dir) == -1) {
        print_error(errno, NULL);
        goto cleanup;
    }
    if((currdir = opendir(".")) == NULL){
        print_error(errno, NULL);
    }

    struct dirent *next_entry;
    errno = 0;
    //Ignore the first two entries of the directory("." and "..")
    if((next_entry = readdir(currdir)) == NULL){
        if (errno != 0){
            print_error(errno, NULL);
            goto cleanup;
        }
    }
    if((next_entry = readdir(currdir)) == NULL){
       if (errno != 0){
           print_error(errno, NULL);
           goto cleanup;
       }
    }
    while((errno = 0, next_entry = readdir(currdir)) != NULL){
        //Ignores hidden or special system files
        if(next_entry->d_name[0] == '.') continue;

        if(strlcpy(path, full_path, MAX_PATH_LEN + 1) > MAX_PATH_LEN){
            print_error(ENAMETOOLONG, NULL);
            goto cleanup;
        }
        if(strlcat(path, "/", MAX_PATH_LEN + 1) > MAX_PATH_LEN){
            print_error(ENAMETOOLONG, NULL);
            goto cleanup;
        }
        if(strlcat(path, next_entry->d_name, MAX_PATH_LEN + 1) > MAX_PATH_LEN){
            print_error(ENAMETOOLONG, NULL);
            goto cleanup;
        }
        if(next_entry->d_type == DT_DIR){
            if(explore_dir(argfiles, path, next_entry->d_name) == -1){
                print_error(errno, NULL);
                goto cleanup;
            }
        }
        else if(next_entry->d_type == DT_REG){
            if(stringlist_put(argfiles, path) == -1){
                print_error(errno, NULL);
                goto cleanup;
            }
        }
        else{
            print_error(EFTYPE, path);
            goto cleanup;
        }
    }
    ret = 0;

    cleanup:
    //Cleanup code to always execute before return
    if (fchdir(owd) == -1) {
        ret = -1;
    }
    if(owd != -1 && close(owd) == -1){
        ret = -1;
    }
    if(currdir != NULL && closedir(currdir) == -1){
        ret = -1;
    }

    return ret;
}

void install_handlers(){
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = usr1_signal_handler;
    sigaction(SIGUSR1, &act, NULL);
    act.sa_handler = term_signal_handler;
    sigaction(SIGHUP, &act, NULL);
    act.sa_handler = term_signal_handler;
    sigaction(SIGINT, &act, NULL);
    act.sa_handler = term_signal_handler;
    sigaction(SIGQUIT, &act, NULL);
    act.sa_handler = term_signal_handler;
    sigaction(SIGTERM, &act, NULL);
    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, NULL);
}

void term_signal_handler(int signo){
    interrupted = 1;
}

void usr1_signal_handler(int signo){
    interrupted_usr1 = 1;
}

