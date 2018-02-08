/*************************************************************************
	> File Name: robust_io.h
	> Author: ye xuefeng
	> Mail: 
	> Created Time: Tue 30 Jan 2018 08:41:49 PM CST
 ************************************************************************/

#ifndef _ROBUST_IO_H
#define _ROBUST_IO_H

#include <stdio.h>

ssize_t ro_read(int fd, void *userbuf, size_t n);
ssize_t ro_write(int fd, void *userbuf, size_t n);
#endif
