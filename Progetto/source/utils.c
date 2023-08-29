#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include "utils.h"
#include "farm.h"


void print_error(int err, char *arg){
    if(err == ELOOP){
        fprintf(stderr, "ERROR: the directory provided might contain a symbolic link loop\n");
    }
    if(err == ENAMETOOLONG){
        fprintf(stderr, "ERROR: some file path provided exceeds the %d length limit for this program,\n"
                        "or some file name exceeds the system lenght limit\n", MAX_PATH_LEN);
    }
    if(err == EACCES){
        fprintf(stderr, "ERROR: access to some file was denied. Check your permissions\n");
    }
    if(err == ENOENT){
        fprintf(stderr, "ERROR: file %s doesn't exist\n", arg);
    }
    if(err == EFTYPE){
        fprintf(stderr, "ERROR: %s is not a file of the appropriate type\n", arg);
    }
    if(err == -1){
        fprintf(stderr, "ERROR: master process crashed unexpectedly\n");
    }
    else{
        fprintf(stderr, "ERROR: an unexpected error occured. Please try again\n");
    }
}

ssize_t writen(int fd, void *ptr, size_t n) {
   size_t   nleft;
   ssize_t  nwritten;

   nleft = n;
   while (nleft > 0) {
     if((nwritten = write(fd, ptr, nleft)) < 0) {
        if (nleft == n) return -1; /* error, return -1 */
        else break; /* error, return amount written so far */
     } else if (nwritten == 0) break;
     nleft -= nwritten;
     ptr   += nwritten;
   }
   return(n - nleft); /* return >= 0 */
}

ssize_t readn(int fd, void *ptr, size_t n) {
   size_t   nleft;
   ssize_t  nread;

   nleft = n;
   while (nleft > 0) {
     if((nread = read(fd, ptr, nleft)) < 0) {
        if (nleft == n) return -1; /* error, return -1 */
        else break; /* error, return amount read so far */
     } else if (nread == 0) break; /* EOF */
     nleft -= nread;
     ptr   += nread;
   }
   return(n - nleft); /* return >= 0 */
}

void sigmask_all(){
    sigset_t nset;
    sigemptyset(&nset);
    sigaddset(&nset, SIGHUP);
    sigaddset(&nset, SIGINT);
    sigaddset(&nset, SIGQUIT);
    sigaddset(&nset, SIGTERM);
    sigaddset(&nset, SIGUSR1);
    pthread_sigmask(SIG_SETMASK, &nset, NULL);
}
void sigunmask_all(){
    sigset_t nset;
    sigemptyset(&nset);
    pthread_sigmask(SIG_SETMASK, &nset, NULL);
}


