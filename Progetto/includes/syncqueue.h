#include <stdlib.h>
#include <stdbool.h>

#ifndef PROJECTSOL_SYNC_QUEUE_H
#define PROJECTSOL_SYNC_QUEUE_H
//Thread safe blocking queue of strings. All sync_queue objects must be accessed
//and modified only with the following functions
//All following functions fail and return -1 if queue == NULL, excepts for sync_queue_destroy
typedef struct sync_queue *sync_queue;

//Initializes the queue. string_size should not account for the terminating character.
//The following functions should each be passed a sync_queue object which was previously
//passed as argument to a sync_queue_init call.
int sync_queue_init(sync_queue *queue, size_t string_size, size_t queue_size);

//Inserts an element in the queue, returning 0 on success. If the string is longer than the queue allows or towrite == NULL,
//then the element is not inserted, and it returns -1.
//When terminate state is set while this thread is waiting on the queue, or if the function is called after
//terminate state is set, return 1.
int sync_queue_put(sync_queue queue, char *towrite);

//Removes an element from the queue, returning 0 on success.
//If dest == NULL returns -1. Dest must be a pointer to a buffer long at least string_size + 1.
//When terminate state is set while this thread is waiting on the queue, or if the function is called after
//terminate state is set, returns 1.
int sync_queue_get(sync_queue queue, char *dest);

//Should be called when queue is no longer to be used.
//Frees any resources held by queue.
void sync_queue_destroy(sync_queue queue);

//Blocks the calling thread until queue becomes empty, and then returns 0
int await_queue_empty(sync_queue queue);

//Sets the terminate status for queue, and returns 0.
//After a successfull call, queue is no longer usable and should be destroyed with sync_queue_destroy
int terminate(sync_queue queue);

#endif //PROJECTSOL_SYNC_QUEUE_H
