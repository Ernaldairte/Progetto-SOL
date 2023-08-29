#include <stdlib.h>
#include <pthread.h>

typedef struct worker_pool{
    pthread_t *threads;
    int dim;
}worker_pool;

int worker_pool_init(worker_pool **pool, void* (*task)(void*), int nthreads, void *arg){
    int ret = 0;
    if(((*pool) = malloc(sizeof(worker_pool))) == NULL)
        goto fail;
    (*pool)->dim = 0;
    if(((*pool)->threads = malloc(sizeof(pthread_t*) * nthreads)) == NULL){
        goto fail;
    }
    //Creates worker threads
    for(int i = 0; i < nthreads; i++){
        if(pthread_create(&(*pool)->threads[i], NULL, task, arg) != 0) goto fail;
        (*pool)->dim++;
    }
    ret = (*pool)->dim;
    goto success;

    fail:
    if(*pool != NULL) {
        ret = (*pool)->dim;
        if ((*pool)->dim == 0) {
            free((*pool)->threads);
            free(*pool);
        }
    }
    success:
    return ret;
}

int worker_pool_destroy(worker_pool *pool, int *term_statuses){
    if(term_statuses == NULL) {
        for (int i = 0; i < pool->dim; i++)
            if (pthread_join(pool->threads[i], NULL) != 0)
                return -1;
    }else{
        for(int i = 0; i < pool->dim; i++)
            if(pthread_join(pool->threads[i], (void *)(term_statuses + i)) != 0)
                return -1;
    }
    return 0;
}


