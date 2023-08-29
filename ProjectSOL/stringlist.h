#ifndef PROJECTSOL_STRINGLIST_H
#define PROJECTSOL_STRINGLIST_H

//Resizable list of fixed length strings, implemented in contiguous memory.
//All functions return 0 on success and -1 on failure.
typedef struct stringlist *stringlist;
//Initialized the stringlist object, for strings long "string_size"
int stringlist_init(stringlist* list , size_t string_size);
//Frees all allocated resources for "list"
int stringlist_destroy(stringlist list);
//Adds the string pointed to by "source" to "list"
int stringlist_put(stringlist list, char* source);
//Removes one string from list copying it to dest
int stringlist_get(stringlist list, char *dest);

#endif //PROJECTSOL_STRINGLIST_H
