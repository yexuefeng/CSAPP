/*************************************************************************
	> File Name: robust_io.c
	> Author: ye xuefeng
	> Mail: 
	> Created Time: Tue 30 Jan 2018 08:43:36 PM CST
 ************************************************************************/
#include "robust_io.h"

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

/*
 * ro_read - a robust wrapper of read. It should take in count these three cases:
 *           1. interrupted by signal handler return; 2. read short count; 3. EOF;
 */
ssize_t ro_read(int fd, void *usrbuf, size_t n)
{
    size_t left = n;
    ssize_t nread;
    char *bufp = usrbuf;
    int count = 0;

    while (left > 0)
    {
        if ((nread = read(fd, bufp, left)) < 0)
        {
            if (errno == EINTR) /* Interrupted by signal handler return */
                nread = 0;
            else if (errno == EAGAIN) /* Resource temporarily unavailable */
                break;
            else
            {
                fprintf(stderr, "%s", strerror(errno));
                *bufp = '\0';
                return -1;
            }
        }
        else if (nread == 0)   /* EOF */
        {
            printf("Peer client closed connection\n");
            *bufp = '\0';
            return -1;
        }
        
        left -= nread;
        bufp += nread;
    }
    
    bufp = '\0'; 
    return (n - left);
}

/*
 * ro_write - a robust wrapper of write. It shuld handle interrupted by signal
 *            handler return;
 */
ssize_t ro_write(int fd, void *usrbuf, size_t n)
{
    size_t left = n;
    ssize_t nwrite;
    char *bufp = usrbuf;

    while (left > 0)
    {
        if ((nwrite = write(fd, bufp, left)) <= 0)
        {
            if (errno == EINTR)
                nwrite = 0;
            else
                return -1;
        }
        left -= nwrite;
        bufp += nwrite;
    }
    return n; 
}
