#include <stdlib.h>
#include <unistd.h>

#define STARTING_SIZE 20
#define UNINITIALIZED 0

typedef struct sck_set{
    int *buf;
    char *types;
    size_t dim;
    size_t size;
}sck_set;

int sck_set_add(sck_set *set, int fd);
int sck_set_init(sck_set **set);
int sck_set_gettype(sck_set *set, int fd);
int sck_set_getmax(sck_set *set);
static int sck_find(sck_set *set, int fd);

int sck_set_init(sck_set **set) {
    if(((*set) = malloc(sizeof(sck_set))) == NULL){
        goto fail;
    }
    (*set)->types = NULL;
    (*set)->buf = NULL;
    (*set)->size = STARTING_SIZE;
    (*set)->dim = 0;

    if(((*set)->types = malloc(sizeof(char) * STARTING_SIZE)) == NULL){
        goto fail;
    }
    for(int i = 0; i < (*set)->size; i++){
        (*set)->types[i] = UNINITIALIZED;
    }
    if(((*set)->buf = malloc(STARTING_SIZE * sizeof(int))) == NULL){
        goto fail;
    }
    return 0;

    fail:
    if(*set != NULL) {
        free((*set)->types);
        free((*set)->buf);
    }
    free(*set);
    return -1;
}
int sck_set_destroy(sck_set *set){
    int ret = 0;
    for(int i = 0; i < set->dim ;i++){
        if(close(set->buf[i]) == -1)
            ret = -1;
    }
    free(set->buf);
    free(set->types);
    free(set);
    return ret;
}
int sck_set_add(sck_set *set, int fd) {
    if(set->size == set->dim){
        void *tmp = realloc(set->buf, set->size * sizeof(int) * 2);
        if(tmp == NULL){
            goto fail;
        }
        set->buf = tmp;
        void* tmp2 = realloc(set->types, set->size * sizeof(char) * 2);
        if(tmp2 == NULL){
            goto fail;
        }
        set->types = tmp2;
        set->size *= 2;
        for(int i = (int)set->dim; i < set->size; i++){
            set->types[i] = UNINITIALIZED;
        }
    }
    int pos = sck_find(set, fd);
    if(pos != -1) goto fail;

    set->buf[set->dim] = fd;
    set->dim++;
    return 0;

    fail:
    return -1;
}
int sck_set_remove(sck_set *set, int fd){
    int pos = sck_find(set, fd);
    if(pos == -1) return -1;
    set->buf[pos] = set->buf[set->dim - 1];
    set->types[pos] = set->types[set->dim -1];
    set->dim--;
    if(close(fd) == -1) return -1;
    return 0;
}
int sck_set_addtype(sck_set *set, int fd, char type){
    int pos = sck_find(set, fd);
    if(pos == -1) return -1;
    set->types[pos] = type;
    return 0;
}

int sck_set_gettype(sck_set *set, int fd) {
    int pos = sck_find(set, fd);
    if(pos == -1){
        return -1;
    }
    return set->types[pos];
}

int sck_set_getmax(sck_set *set) {
    int max = -1;
    for(int i = 0; i < set->dim; i++){
        if(set->buf[i] > max)
            max = set->buf[i];
    }
    return max;
}

static int sck_find(sck_set *set, int fd) {
    for(int i = 0; i < set->dim; i++){
        if(set->buf[i] == fd){
            return i;
        }
    }
    return -1;
}








