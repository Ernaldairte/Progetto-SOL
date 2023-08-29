#ifndef PROJECTSOL_LIBFARM_H
#define PROJECTSOL_LIBFARM_H


#include "stringlist.h"
#include "syncqueue.h"
#include "workerpool.h"

#define MAX_PATH_LEN 255
#define THREADS_DEFAULT_N 4
#define QUEUE_DEFAULT_SZ 8
#define WORKER_MSG_SZ (sizeof(long) + MAX_PATH_LEN + 1)
#define MASTER_MSG_SZ 1
#define PRINT 1
#define QUIT 0
#define TYPE_SZ 1
#define TYPE_MASTER 1
#define TYPE_WORKER 2

int master(int argc, char *argv[]);
int collector();

extern volatile sig_atomic_t interrupted;
extern volatile sig_atomic_t interrupted_usr1;

#endif //PROJECTSOL_LIBFARM_H
