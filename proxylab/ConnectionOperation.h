/*************************************************************************
	> File Name: ConnectionOperation.h
	> Author: ye xuefeng
	> Mail: yexuefeng_coder@outlook.com
	> Created Time: 2018年02月09日 星期五 18时02分11秒
 ************************************************************************/

#ifndef _CONNECTIONOPERATION_H
#define _CONNECTIONOPERATION_H

#include <fcntl.h>

#include "dlist.h"
#include "cache.h"
#include "csapp.h"

#define HTTP_URL_LEN 256
#define HTTP_METHOD_LEN 64
#define HTTP_VERSION_LEN 64 

struct connection;

typedef enum _State {
    HALF_CONNECTION, /*Just create connect between client and proxy */
    ALL_CONNECTION, /*Both connections among client, proxy and server are created*/
    HALF_FINISH_CONNECTION, /* Pair connection has freeed */
    FINISH_CONNECTION /*Connection going to be closed*/
} State;

/*
 * client <--------------------> proxy <--------------------> server
 * Every http request needs two TCP connections. One is the connection between
 * client and proxy, the other is the connection between proxy and server.
 */
struct connection {
    int fd; 
    char data[MAX_OBJECT_SIZE];
    int size;
    int first, last;
    struct connection *pair;
    State state;
};

int add_epoll_event(int epfd, int fd);
int set_fd_nonblock(int fd);
DList* init_connection_table();
void deinit_connection_table(DList* connectionTable);
struct connection* find_connection(DList* connectionTable, int fd);
Ret delete_connection(DList *connectionTable, struct connection *conn);

/*
 * read_from_connection - read data from connection. return 0 if everything is ok,
 *                        -1 if error occurs, -2 if connection closed.
 */
int read_from_connection(struct connection *conn, int epfd);

int read_from_half_connection(DList *connectionTable,
                              struct connection* conn,
                              int epfd);
int get_new_connection(DList* connectionTable, int epfd, int fd);

/*
 * write_to_connection - write data to connection. 
 */
int write_to_connection(struct connection *conn, int epfd);
#endif
