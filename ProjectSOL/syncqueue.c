#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

typedef struct sync_queue{
    char **buf;
	size_t size;
    size_t max_size;
	size_t start;
	size_t end;
    size_t dim;
    pthread_mutex_t queue_mtx;
	pthread_cond_t not_full_term;
	pthread_cond_t not_empty_term;
    pthread_cond_t empty;
    bool terminate;
}sync_queue;

int sync_queue_init(sync_queue**, size_t, size_t);
void sync_queue_destroy(sync_queue *queue);
int sync_queue_put(sync_queue *queue, char *);
int sync_queue_get(sync_queue *queue, char *);

int sync_queue_init(sync_queue **queue, size_t string_size, size_t nelem){
    int i = 0;
    if((*queue = malloc(sizeof(sync_queue))) == NULL){
        goto end;
    }
    (*queue)->size = nelem;
    (*queue)->max_size = string_size + 1;
    (*queue)->start = 0;
    (*queue)->end = 0;
    (*queue)->dim = 0;
    (*queue)->buf = NULL;

    if(((*queue)->buf = malloc(sizeof(char*) * (*queue)->size)) == NULL){
        goto free;
    }
    for(; i < (*queue)->size; i++){
        if(((*queue)->buf[i] = malloc((*queue)->max_size)) == NULL){
            goto free;
        }
    }
	if(pthread_mutex_init(&(*queue)->queue_mtx, NULL) != 0){
        goto clean_mtx;
    }
    if(pthread_cond_init(&(*queue)->not_empty_term, NULL) != 0){
        goto clean_notempty;
    }
    if(pthread_cond_init(&(*queue)->not_full_term, NULL) != 0){
        goto clean_notfull;
    }
    if(pthread_cond_init(&(*queue)->empty, NULL) != 0){
        goto clean_empty;
    }
    return 0;

    clean_empty:
    pthread_cond_destroy(&(*queue)->empty);

    clean_notfull:
    pthread_cond_destroy(&(*queue)->not_full_term);

    clean_notempty:
    pthread_cond_destroy(&(*queue)->not_empty_term);

    clean_mtx:
    pthread_mutex_destroy(&(*queue)->queue_mtx);

    free:
    for(int j = 0; j < i; j++){
        free((*queue)->buf[i]);
    }
    free((*queue)->buf);
    queue = NULL;

    end:
    return -1;
}

void sync_queue_destroy(sync_queue *queue){
    if(queue != NULL){
        for (int i = 0; i < queue->size; i++) {
            free(queue->buf[i]);
        }
        free(queue->buf);
        pthread_mutex_destroy(&queue->queue_mtx);
        pthread_cond_destroy(&queue->not_empty_term);
        pthread_cond_destroy(&queue->not_full_term);
        pthread_cond_destroy(&queue->empty);
        free(queue);
    }
}

int sync_queue_put(sync_queue *queue, char *source){
    int ret = 0;
    if(queue == NULL || source == NULL){
        return -1;
    }
    pthread_mutex_lock(&queue->queue_mtx);
	while(queue->dim == queue->size && queue->terminate == false){
		pthread_cond_wait(&queue->not_full_term, &queue->queue_mtx);
	}
    if(queue->terminate){
        ret = 1;
        goto end;
    }
    strlcpy(queue->buf[queue->end], source, queue->max_size);

	queue->end++;
	queue->end %= queue->size;
    queue->dim++;

    end:
    pthread_mutex_unlock(&queue->queue_mtx);
	pthread_cond_signal(&queue->not_empty_term);
    return ret;
}

int sync_queue_get(sync_queue *queue, char *dest){
    int ret = 0;
    pthread_mutex_lock(&queue->queue_mtx);
	while(queue->dim == 0 && queue->terminate == false){
        pthread_cond_wait(&queue->not_empty_term, &queue->queue_mtx);
	}
    if(queue->terminate){
        ret = 1;
        goto end;
    }
    strlcpy(dest, queue->buf[queue->start], queue->max_size);
    queue->start++;
    queue->start %= queue->size;
    queue->dim--;

    end:
    pthread_mutex_unlock(&queue->queue_mtx);
    if(queue->dim == 0)
        pthread_cond_signal(&queue->empty);
    pthread_cond_signal(&queue->not_full_term);
    return ret;
}

int terminate(sync_queue *queue){
    if(queue == NULL)
        return -1;
    pthread_mutex_lock(&queue->queue_mtx);
    queue->terminate = true;
    pthread_mutex_unlock(&queue->queue_mtx);
    pthread_cond_broadcast(&queue->not_full_term);
    pthread_cond_broadcast(&queue->not_empty_term);
    return 0;
}

int await_queue_empty(sync_queue *queue){
    if(queue == NULL)
        return -1;
    pthread_mutex_lock(&queue->queue_mtx);
    while(queue->dim > 0){
        pthread_cond_wait(&queue->empty, &queue->queue_mtx);
    }
    pthread_mutex_unlock(&queue->queue_mtx);
    return 0;
}




