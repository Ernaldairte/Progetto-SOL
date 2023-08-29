#include <stdlib.h>
#include <string.h>

#define DEF_LIST_SZ 10

typedef struct stringlist{
    void *strings;
    size_t dim;
    size_t size;
    size_t elem_size;
}stringlist;

int stringlist_init(stringlist **list, size_t string_size);
int stringlist_destroy(stringlist *list);
int stringlist_put(stringlist *list, char *toput);
size_t stringlist_getdim(stringlist *list);
int stringlist_get(stringlist *list, char *dest);

int stringlist_init(stringlist **list, size_t string_size) {
    (*list) = malloc(sizeof(stringlist));
    if(*list == NULL){
        goto fail;
    }
    (*list)->dim = 0;
    (*list)->size= DEF_LIST_SZ;
    (*list)->elem_size = string_size + 1;
    (*list)->strings = malloc(sizeof(char[(*list)->size][(*list)->elem_size]));
    if((*list)->strings == NULL){
        goto fail;
    }
    return 0;

    fail:
    free((*list)->strings);
    free(*list);
    return -1;
}

int stringlist_destroy(stringlist *list){
    if(list == NULL) return -1;
    free(list->strings);
    free(list);
    return 0;
}

int stringlist_put(stringlist *list, char* toput){
    char (*tstrings)[][list->elem_size];
    if(list == NULL) return -1;
    if(list->dim == list->size){
        void *res;
        size_t new_size = list->size * 2;
        if ((res = realloc(list->strings, sizeof(char[new_size][list->elem_size]))) == NULL){
            return -1;
        }
        list->strings = res;
        list->size = new_size;
    }
    tstrings = list->strings;
    if(strlcpy((*tstrings)[list->dim], toput, list->elem_size) > list->elem_size){
        return -1;
    }
    list->dim++;
    return 0;
}


int stringlist_get(stringlist *list, char *dest) {
    if(list == NULL){
        return -1;
    }
    if(list->dim == 0){
        return -1;
    }
    char (*tstrings)[][list->elem_size] = list->strings;
    strlcpy(dest, (*tstrings)[--(list->dim)], list->elem_size);
    return 0;
}
