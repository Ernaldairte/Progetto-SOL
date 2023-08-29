#ifndef PROJECTSOL_WORKERPOOL_H
#define PROJECTSOL_WORKERPOOL_H

typedef struct worker_pool *worker_pool;

//Create a pool of threads, with argument "arg", executing "task".
//Returns the number of threads created successfully.
int worker_pool_init(worker_pool *pool, void* (*task)(void*), int nthreads, void *arg);

//Waits for termination of all threads in the pool, freeing all resources allocated to them, and saves return codes in "term_statuses".
//Term statuses must be a pointer to an int buffer long at least the number of threads.\
//Returns 0 on success, -1 if some error occured.
int worker_pool_destroy(worker_pool pool, int *term_statuses);

#endif //PROJECTSOL_WORKERPOOL_H
