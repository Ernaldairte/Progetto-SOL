#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "farm.h"
#include "utils.h"
#include "sckset.h"
#include "orderedrecordlist.h"

static int create_socket();

int main(){
    collector();
}

int collector(){
    sigmask_all();
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = SIG_IGN;
    sigaction(SIGHUP, &act, NULL);

//    printf("Collector in\n");
    int ret = -1;
    int fd_acceptsck = create_socket();
    if(fd_acceptsck == -1){
        goto end;
    }
    ordered_recordlist list;
    if(ordered_recordlist_init(&list, MAX_PATH_LEN) == -1){
        goto clean_fd;
    }
    //Used to associate a type to sockets and to support
    //operations on fd_set set
    sck_set s_set;
    if(sck_set_init(&s_set) == -1){
        goto clean_list;
    }
    int fd_num = 0;
    fd_set set, rdset;
    if (fd_acceptsck > fd_num) fd_num = fd_acceptsck;
    FD_ZERO(&set);
    FD_SET(fd_acceptsck, &set);

    //Timeout in case master process dies before connecting
    struct timeval timeout;
    timeout.tv_sec = 300;
    timeout.tv_usec = 0;

    while(true){
        rdset = set;
        int fd_ready;
        fd_ready = select(fd_num + 1, &rdset, NULL, NULL, &timeout);
        if(fd_ready == -1){
            goto clean_all;
        }
        if(fd_ready == 0){
            print_error(-1, NULL);
            goto clean_all;
        }
        int fd_curr = 0;
        int fds_found = 0;
        while(fds_found < fd_ready && fd_curr < fd_num + 1){
            if(!FD_ISSET(fd_curr, &rdset)){
                fd_curr++;
                continue;
            }
            else if(fd_curr == fd_acceptsck){
                int tmp_fd = accept(fd_acceptsck, NULL, 0);
                if(tmp_fd == -1){
                    goto clean_all;
                }
                sck_set_add(s_set, tmp_fd);
                FD_SET(tmp_fd, &set);
                if(tmp_fd > fd_num) fd_num = tmp_fd;
//                printf("Connected a client\n");
            }
            else{
                int type = sck_set_gettype(s_set, fd_curr);
                //If the client associated to fd_curr has no type yet,
                //the data that just arrived is its type
                if(type == UNINITIALIZED){
                    ssize_t nread;
                    char msg;
                    nread = readn(fd_curr, &msg, TYPE_SZ);
                    if(nread == 0){ //Client closed connection, remove it from fd_set and close the socket
                        FD_CLR(fd_curr, &set);
                        sck_set_remove(s_set, fd_curr);
                        fd_num = sck_set_getmax(s_set) + 1;
                    }
                    else if(nread != TYPE_SZ){ //Error occured
                        goto clean_all;
                    }
                    else{
                        sck_set_addtype(s_set, fd_curr, (char) msg);
                    }
                }
                else if(type == TYPE_MASTER){  //If type is "master", terminate on "QUIT" and print record on "PRINT"
                    ssize_t nread;
                    char msg;
                    nread = readn(fd_curr, &msg, MASTER_MSG_SZ);
                    if(nread == 0){ //Master process crashed
                        print_error(-1, NULL);
                        goto clean_all;
                    }
                    if(nread != MASTER_MSG_SZ){ //Error occured
                        goto clean_all;
                    }
                    if(msg == PRINT){
//                        printf("Received master PRINT\n");
                        ordered_recordlist_printall(list);
                    }
                    else if(msg == QUIT){
//                        printf("Received master QUIT\n");
                        ordered_recordlist_printall(list);
                        ret = 0;
                        goto clean_all;
                    }
                }
                else{ //If type is worker, add filename and its value to list
                    ssize_t nread;
                    char msg[WORKER_MSG_SZ];
                    nread = readn(fd_curr, &msg, WORKER_MSG_SZ);
                    if(nread == 0){ //Worker process closed, remove it from fd_set
                        FD_CLR(fd_curr, &set);
                        sck_set_remove(s_set, fd_curr);
                        fd_num = sck_set_getmax(s_set) + 1;
                    }
                    else if(nread != WORKER_MSG_SZ){ //Error occured
                        goto clean_all;
                    }
                    else{
                        long val = *((long *) msg);
                        char name[MAX_PATH_LEN + 1];
                        strlcpy(name, msg + sizeof(val), MAX_PATH_LEN + 1);
                        ordered_recordlist_add(list, name, val);
//                        printf("Message from worker received:\n%lu %s\n", val, name);
                    }
                }
            }
            fd_curr++;
            fds_found++;
        }
    }
    clean_all:
    sck_set_destroy(s_set);

    clean_list:
    ordered_recordlist_destroy(list);

    clean_fd:
    close(fd_acceptsck);

    end:
    unlink("farm.sck");
//    printf("Collector out\n");
    return ret;
}

static int create_socket() {
    int fd_acceptsck;
    struct sockaddr_un sa;
    strlcpy(sa.sun_path, "farm.sck", sizeof(sa.sun_path));
    sa.sun_family = AF_UNIX;
    if((fd_acceptsck = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
        goto failure;
    }
    if(bind(fd_acceptsck, (struct sockaddr *) &sa, sizeof(sa)) == -1){
        goto failure;
    }
    if(listen(fd_acceptsck, SOMAXCONN) == -1){
        goto failure;
    }
    return fd_acceptsck;

    failure:
    if(fd_acceptsck != 0){
        if(close(fd_acceptsck) == -1) print_error(errno, NULL);
    }
    return -1;
}

