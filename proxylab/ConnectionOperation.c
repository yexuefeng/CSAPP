/*************************************************************************
	> File Name: ConnectionOperation.c
	> Author: ye xuefeng
	> Mail: yexuefeng_coder@outlook.com
	> Created Time: 2018年02月09日 星期五 18时01分57秒
 ************************************************************************/

#include "ConnectionOperation.h"

#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <sys/epoll.h>
#include <string.h>

#include "dlist.h"
#include "typedef.h"
#include "csapp.h"


/* little helper function */
static int compare(void *iter, void *ctx)
{
    int fd = (int)ctx;
    DListNode *node = (DListNode*)iter;
    struct connection *conn = (struct connection*)(node->data);
    return conn->fd == fd ? 0 : -1;
}

static void free_connection(void *ctx, void *connection)
{
    struct connection* conn = (struct connection *)connection;
    if (conn)
    {
        close(conn->fd);
        if (conn->pair)
            conn->pair->pair = NULL;
        free(conn);
    } 
}

int disable_write(int epfd, int fd)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
    {
        fprintf(stderr, "epoll_ctl error\n");
        return -1;
    }

    return 0;
}

int enable_write(int epfd, int fd)
{
    struct epoll_event ev;
    
    printf("%s(%d)\n Enable write of fd %d", __func__, __LINE__, fd); 
    memset(&ev, 0, sizeof(ev));
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLOUT;
    if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
    {
        fprintf(stderr, "epoll_ctl error\n");
        return -1;
    }

    return 0;
}

int add_epoll_event(int epfd, int fd)
{
    struct epoll_event ev;
    
    memset(&ev, 0, sizeof(ev));
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
    {
        fprintf(stderr, "epoll_ctl error\n");
        return -1;
    }

    printf("Add file descriptor %d to watch\n", fd);

    return 0;
}

int set_fd_nonblock(int fd)
{
    int flags;
    int ret;
    
    if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
    {
        fprintf(stderr, "fcntl error, %s\n", strerror(errno));
        return -1;
    }

    flags |= O_NONBLOCK;
    if ((ret = fcntl(fd, F_GETFL, flags)) == -1)
    {
        fprintf(stderr, "fcntl error, %s\n", strerror(errno));
        return -1;
    }

    return ret;
}


DList *init_connection_table()
{
    DList *connection_table = dlist_create(free_connection, NULL);
    return connection_table;
}

void deinit_connection_table(DList *connectionTable)
{
    dlist_destroy(connectionTable); 
}

/*
 * connection_find - find the corresponding connection data of a socket descriptor.
 *                   return NULL if not found.
 */
struct connection* find_connection(DList *connectionTable, int fd)
{
    int index;
    DListNode* node;
    struct connection *conn;

    index = dlist_find(connectionTable, compare, (void*)fd);
    if (index < 0)
        return NULL;

    node = dlist_get_node(connectionTable, index, 0);
    conn = node->data;
    if(conn)
    {
        (void)conn;
        assert(conn->fd == fd);
    }

    return conn;
}

/*
 * insert_connection - Insert a new connection to the connection table
 */
Ret append_connection(DList *connectionTable, struct connection *conn)
{
    return dlist_append(connectionTable, conn); 
}

/*
 * delete_connection - Delete a new connection to the connection table
 */
Ret delete_connection(DList *connectionTable, struct connection *conn)
{
    struct connection *data;    
    int index = dlist_find(connectionTable, compare, (void*)conn->fd);
    data = dlist_delete(connectionTable, index);
    assert(data == conn);
    return data != NULL ? RET_OK : RET_FAIL;
}

/*
 * make_connection - make a new connection node in the list
 */
void* make_connection(int fd)
{
    struct connection *conn;
    if ((conn = malloc(sizeof(struct connection))) != NULL)
    {
        conn->fd = fd;
        conn->size = 0;
        conn->first = conn->last = 0;
        conn->pair = NULL;
        return conn;
    }
    
    return NULL;
}

/*
 * parse_url - Parse the url in the http request. Return 0 if success, or -1
 *             if the request is malformed or malicious.
 */
static int parse_url(const char *request, char *hostname, char *uri, char *port)
{
    char buf[MAXLINE];
    char *first, *ptr, *end;
    
    strncpy(buf, request, MAXLINE);
    end = buf + strlen(buf);
    if ((first = strstr(buf, "//")))
    {
        first += 2;
        /* Parse hostname and port */
        if ((ptr = strchr(first, ':')))
        {
            *ptr = ' ';
            if ((ptr = strchr(first, '/')))
            {
                *ptr = '\0';
            }
            sscanf(first, "%s %s", hostname, port);
        }
        else
        {
            if ((ptr = strchr(first, '/')))
            {
                *ptr = '\0';
            }
            sscanf(first, "%s", hostname);
            strcpy(port, "80");
        }
        
        /* Parse uri */
        strcpy(uri, "/");
        if (ptr != NULL && ptr < end)
        {
            strcat(uri, ptr + 1);
        }
        
        return 0;
    }
    
    return -1;
}

/*
 * read_from_connection - Read data from the connection, remember the data would send
 *                        to the pair connection. So we need store the data into the 
 *                        pair connection buffer.
 */
int read_from_connection(struct connection *conn, int epfd)
{
    struct connection* pair = conn->pair;
    struct epoll_event ev;
    int fd = conn->fd;
    ssize_t nread;
    
    /* No space left, just return*/ 
    if (!pair || MAX_OBJECT_SIZE == pair->size)
        return 0;

    nread = read(fd, pair->data + pair->last, MAX_OBJECT_SIZE - pair->size);
    if (nread < 0)
    {
        if (errno == EINTR || errno == EAGAIN)
            return 0;
        else
            return -1;
    }
    else if (nread == 0)
    {
       return -2;
    }
    else
    {
        pair->last = (pair->last + nread) % MAX_OBJECT_SIZE;
        pair->size += nread;
        pair->data[pair->size] = '\0';
        
        /* 
         * As we have data need to send to pair connection.
         * Enable write of the pair connection 
         */
        ev.data.fd = pair->fd;
        ev.events = EPOLLIN | EPOLLOUT;
        if (epoll_ctl(epfd, EPOLL_CTL_MOD, pair->fd, &ev) == -1)
        {
            fprintf(stderr, "epoll_ctl error\n");
            return -1;
        }
        return 0;
    } 
}

/*
 * connect_to_server
 */
struct connection* connect_to_server(DList* connectionTable, const char* url, struct connection* conn)
{
        char hostname[64];
        char port[16];
        char uri[HTTP_URL_LEN];
        int clientfd;
        struct connection* pair;

        if (parse_url(url, hostname, uri, port) == -1)
        {
            return NULL;
        }
        
        if ((clientfd = open_clientfd(hostname, port)) < 0)
        {
            fprintf(stderr, "open_clientfd error\n");
            return NULL;
        } 

        if (set_fd_nonblock(clientfd) == -1)
        {
            fprintf(stderr, "set_fd_nonblock error\n");
            return NULL;
        }

        if ((pair = make_connection(clientfd)) == NULL)
        {
            fprintf(stderr, "make_connection error\n");
            return NULL;
        }

        conn->pair = pair;
        pair->pair = conn;
        conn->state = ALL_CONNECTION;
        pair->state = ALL_CONNECTION;
        
        printf("pair: fd1, %d, fd2, %d\n", conn->fd, pair->fd);
        return pair;
}

/*
 * int read_from_half_connection
 */
int read_from_half_connection(DList* connectionTable, struct connection* conn, int epfd)
{
    char request[MAXLINE];
    ssize_t nread;
    int fd = conn->fd;
    char method[HTTP_METHOD_LEN];
    char version[HTTP_VERSION_LEN];
    char url[HTTP_URL_LEN];
    char* content;
    struct epoll_event ev;
    struct connection* pair;

    assert(conn->pair == NULL && conn->state == HALF_CONNECTION);
    
    nread = read(fd, request, MAXLINE);
    if (nread < 0)
    {
        if (errno == EINTR || errno == EAGAIN)
            return 0;
        else
            return -1;
    }
    else if (nread == 0)
    {
       return -2;
    }
    else
    {
        sscanf(request, "%s %s %s",  method, url, version);
        
        /* First we find request in the cache */
        if (!strcasecmp(method, "GET"))
            content = search_in_cache(url);

        if (content)
        {
            int len;
            if (strlen(content) > MAX_OBJECT_SIZE - conn->size)
                len = MAX_OBJECT_SIZE - conn->size;
            else
                len = strlen(content);
            strncpy(conn->data, content, len);
            conn->size += len;
            conn->last = (conn->last + len) % MAX_OBJECT_SIZE;
            free(content);
            
            ev.data.fd = conn->fd;
            ev.events = EPOLLIN | EPOLLOUT;
            if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
            {
                fprintf(stderr, "epoll_ctl error\n");
                return -1;
            }
        }
        else
        {
            pair = connect_to_server(connectionTable, url, conn);
            if (!pair)
            {
                fprintf(stderr, "connect_to_server failed\n");
                return -1;
            }
        }
        return 0;
    }
}

/*
 * write_to_connection - Write data to the connection. If all data have sent to the 
 *                       connection, we should disable the write of the connection.
 */
int write_to_connection(struct connection *conn, int epfd)
{
    int fd = conn->fd;
    ssize_t  nwrite;
    struct epoll_event ev;
    
    nwrite = write(fd, conn->data + conn->first, conn->size);
    if (nwrite < 0)
    {
        printf("error happened, %s\n", strerror(errno));
        if (errno == EINTR || errno == EAGAIN)
            return 0;
        else
            return -1;
    }
    else
    {
        conn->first = (conn->first + nwrite) % MAX_OBJECT_SIZE;
        conn->size -= nwrite;
        if (conn->size == 0)
        {
            assert(conn->first == conn->last);
            conn->first = 0;
            conn->last = 0;
        }
        return 0;
    }
}


/*
 * get_new_connection - Get a new request from client. First we should search in proxy cache to find
 *                      the corresponding cached content. Second if the cache missed, we should
 *                      create connection between the proxy and the web server. Third, we should
 *                      forward the request to the final web server. Fourth we should cache the 
 *                      new content.
 */
int get_new_connection(DList* connectionTable, int epfd, int fd)
{
    char request[MAX_OBJECT_SIZE];
    char *content = NULL;
    ssize_t nread;
    char method[HTTP_METHOD_LEN];
    char version[HTTP_VERSION_LEN];
    char url[HTTP_URL_LEN];
    struct epoll_event ev;
    struct connection* pair;
    struct connection* conn = make_connection(fd); 
     
    if (!conn)
        return -1;
     
    conn->state = HALF_CONNECTION;
    nread = read(fd, request, MAX_OBJECT_SIZE);
    if (nread < 0)
    {
        if (errno == EINTR)
            return 0;
        else
            return -1;
    }
    else if (nread == 0)
    {
        close(fd);
        free(conn);
        return 0;
    }
    else
    {
        append_connection(connectionTable, conn);

        sscanf(request, "%s %s %s", method, url, version);
        if (!strcasecmp(method, "GET"))
        {
            content = search_in_cache(url);    
        }
        
        if (content)
        {
            int len;
            if (strlen(content) > MAX_OBJECT_SIZE - conn->size)
                len = MAX_OBJECT_SIZE - conn->size;
            else
                len = strlen(content);
            strncpy(conn->data, content, len);
            conn->size += len;
            conn->last = (conn->last + len) % MAX_OBJECT_SIZE;
            free(content);
            
            ev.data.fd = conn->fd;
            ev.events = EPOLLIN | EPOLLOUT;
            if (epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev) == -1)
            {
                fprintf(stderr, "epoll_ctl error\n");
                return -1;
            }
        }
        else /* Need to connect to server */
        {
            ssize_t nwrite;
            pair = connect_to_server(connectionTable, url, conn);
            if (!pair)
            {
                fprintf(stderr, "connect_to_server failed\n");
                return -1;
            }
            strncpy(pair->data, request, nread);
            pair->size += nread;
            pair->last = (pair->last + nread) % MAX_OBJECT_SIZE;
           
            add_epoll_event(epfd, pair->fd);
            enable_write(epfd, pair->fd);
            append_connection(connectionTable, pair);
        }
    }

    return 0;
}

