#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
void *task(void * arg);
void handle(int arg);

int main(){
    struct sigaction siga;
    memset(&siga, 0, sizeof(siga));
    siga.sa_handler = handle;
    sigaction(SIGQUIT, &siga, NULL);
    pthread_t thread;
    pthread_create(&thread, 0, task, NULL);
    for(int i = 0; i < 1000; i++){
        pthread_kill(thread, SIGQUIT);
        sleep(1);
    }
    pthread_join(thread, NULL);
    printf("Ended\n");
    return 0;
}
void *task(void * arg){
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    errno = 0;
    int res;
    int i = 0;
    while(i < 1000){
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond, &mutex);
        pthread_mutex_unlock(&mutex);
        i++;
        printf("%d %d\n", i, res);
    }
    return 0;
}

void handle(int arg){
}