#ifndef PROJECTSOL_SCKSET_H
#define PROJECTSOL_SCKSET_H

#define UNINITIALIZED 0

#define UNINITIALIZED 0

//Data structure that holds a set of socket file descriptors.
//Each socket can have a type associated with it, ranging from 1 to 255.
//All functions return 0 on success (except for sck_max and gettype) and -1 on failure
typedef struct sck_set *sck_set;
//Initilized "set"
int sck_set_init(sck_set *set);
//Frees all resources allocated for "set" and closes all file descriptors
int sck_set_destroy(sck_set set);
//Adds "fd" to "set"
int sck_set_add(sck_set set, int fd);
//Returns the type of "fd"
int sck_set_gettype(sck_set set, int fd);
//Adds type "type" to "fd"
int sck_set_addtype(sck_set set, int fd, char type);
//Returns the highest file descriptor
int sck_set_getmax(sck_set set);
//Removes "fd" from "set", closing it
int sck_set_remove(sck_set set, int fd);

#endif //PROJECTSOL_SCKSET_H
