#ifndef PROJECTSOL_UTILS_H
#define PROJECTSOL_UTILS_H

#include <sys/types.h>

#define PRINT_USAGE printf("usage: %s [-n nthread] [-q qlen] [-t delay] [-d dir_name] path1 [path2 ...]\n", argv[0]); \
					printf("       %s [-n nthread] [-q qlen] [-t delay] -d dir_name [path1 ...]\n", argv[0]);         \
                    printf("where n, q, t are positive integers\n")

void print_error(int err, char *arg);
ssize_t writen(int fd, void *ptr, size_t n);
ssize_t readn(int fd, void *ptr, size_t n);
void sigmask_all();
void sigunmask_all();
void term_signal_handler(int signo);
void usr1_signal_handler(int signo);

#endif //PROJECTSOL_UTILS_H

