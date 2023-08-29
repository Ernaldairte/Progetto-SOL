#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_SIZE 10
typedef struct record{
    char *name;
    long value;
}record;

typedef struct ordered_recordlist{
    record *buf;
    size_t name_size;
    size_t size;
    size_t dim;
}ordered_recordlist;

int ordered_recordlist_init(ordered_recordlist **list, int name_size);
int ordered_recordlist_destroy(ordered_recordlist *list);
int ordered_recordlist_add(ordered_recordlist *list, char *name, long value);
void ordered_recordlist_printall(ordered_recordlist *list);
static int compare_record(const void *arg1, const void *arg2);

int ordered_recordlist_init(ordered_recordlist **list, int name_size){
    int i = 0;
    if((*list = malloc(sizeof(ordered_recordlist))) == NULL) goto fail;
    (*list)->name_size = name_size;
    (*list)->dim = 0;
    (*list)->size = DEFAULT_SIZE;
    if(((*list)->buf = malloc(sizeof(record) * DEFAULT_SIZE)) == NULL) goto fail;

    for(; i < (*list)->size; i++){
        void *tmp = malloc(name_size + 1);
        if(tmp == NULL) goto fail;
        (*list)->buf[i].name = tmp;
    }
    return 0;

    fail:
    if(*list != NULL && (*list)->buf != NULL)
        for(int j = 0; j < i; j++){
            free((*list)->buf[i].name);
        }
    if(*list != NULL)
        free((*list)->buf);
    free(*list);
    return -1;
}

int ordered_recordlist_destroy(ordered_recordlist *list){
    for(int i = 0; i < list->size; i++){
        free(list->buf[i].name);
    }
    free(list->buf);
    free(list);
    return 0;
}

int ordered_recordlist_add(ordered_recordlist *list, char *name, long value){
    int i = (int)list->size;
    if(list->dim == list->size){
        void *tmp = realloc(list->buf, sizeof(record) * list->size * 2);
        if(tmp == NULL) goto fail;
        list->buf = tmp;
        list->size *= 2;
        for(; i < list->size; i++){
            void *tmp2 = malloc(list->name_size + 1);
            if(tmp2 == NULL) goto fail;
            list->buf[i].name = tmp2;
        }
    }
    strlcpy(list->buf[list->dim].name, name, list->name_size + 1);
    list->buf[list->dim].value = value;
    list->dim++;
    qsort(list->buf, list->dim, sizeof(record), compare_record);
    return 0;

    fail:
    for(int j = i; j < list->size; j++){
        list->buf[i].name = NULL;
    }
    return -1;
}

void ordered_recordlist_printall(ordered_recordlist *list){
    for(int i = 0; i < list->dim; i++){
        printf("%lu %s\n", list->buf[i].value, list->buf[i].name);
    }
}

static int compare_record(const void *arg1, const void *arg2){
    record *a = (record*)arg1;
    record *b = (record*)arg2;
    if(a->value < b->value) return -1;
    if(a->value > b->value) return 1;
    else return 0;
}




