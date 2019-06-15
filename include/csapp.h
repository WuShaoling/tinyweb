#ifndef __CSAPP_H__
#define __CSAPP_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct sockaddr SA;

#define RIO_BUFSIZE 8192
typedef struct {
    int rio_fd;                /* descriptor for this internal buf */
    int rio_cnt;               /* unread bytes in internal buf */
    char *rio_bufptr;          /* next unread byte in internal buf */
    char rio_buf[RIO_BUFSIZE]; /* internal buffer */
} rio_t;

extern char **environ; /* defined by libc */

/* Misc constants */
#define MAXLINE  8192  /* max text line length */
#define MAXBUF   8192  /* max I/O buffer size */
#define LISTENQ  1024  /* second argument to listen() */

void unix_error(char *msg);

pid_t Fork(void);

void Execve(const char *filename, char *const argv[], char *const envp[]);

pid_t Wait(int *status);

int Open(const char *pathname, int flags, mode_t mode);

void Close(int fd);

int Dup2(int fd1, int fd2);

void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);

void Munmap(void *start, size_t length);

int Accept(int s, struct sockaddr *addr, socklen_t *addrlen);

ssize_t rio_writen(int fd, void *usrbuf, size_t n);

void rio_readinitb(rio_t *rp, int fd);

ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

void Rio_writen(int fd, void *usrbuf, size_t n);

void Rio_readinitb(rio_t *rp, int fd);

ssize_t Rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

int open_listenfd(int portno);

int Open_listenfd(int port);

#endif /* __CSAPP_H__ */
