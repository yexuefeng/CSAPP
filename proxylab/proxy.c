#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <sys/epoll.h>

#include "csapp.h"
#include "cache.h"
#include "queue.h"
#include "ConnectionOperation.h"

/* Recommended max cache and object sizes */

#define THREAD_NUM 4
#define MAX_FILENO_PER_THREAD 64
#define MAX_EVENTS MAX_FILENO_PER_THREAD

pthread_t tid[THREAD_NUM];


void* proxy_thread(void *argv);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

void make_request_packet(char *request,
                         char *method,
                         char *hostname,
                         char *uri,
                         char *port)
{
    sprintf(request, "%s %s HTTP/1.0\r\n", method, uri);
    sprintf(request, "%sHost: %s\r\n", request, hostname);
    sprintf(request, "%s%s", request, user_agent_hdr);
    sprintf(request, "%sConnection: close\r\n", request);
    sprintf(request, "%sProxy-Connection: close\r\n\r\n", request);
}

/*
 * Reuse socket address.
 */
void set_socket_reuse(int listenfd)
{
    int optval = 1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
                   &optval, sizeof(int)) < 0)
        err_exit("setsockopt error");
}

static void display_usage(const char *progname)
{
    fprintf(stderr, "%s <port>\n", progname);
    exit(-1);
}

int main(int argc, char *argv[])
{
    int i;
    int listenfd, connfd;
    struct sockaddr_in clientaddr;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t addrlen;
    Queue *queue;
    Queue **queue_array;
    int ret;
    int index = 0;

    if (argc < 2)
    {
        display_usage(argv[0]);
    }
    
    if ((listenfd = open_listenfd(argv[1])) < 0)
    {
        err_exit("open_listenfd error");
    } 
    set_socket_reuse(listenfd);

    init_cache();
    signal(SIGPIPE, SIG_IGN);

    /* Create thread pool */
    queue_array = malloc(sizeof(Queue*));
    for (i = 0; i < THREAD_NUM; i++)
    {
        queue = queue_create(NULL, NULL);
        queue_array[i] = queue;
        pthread_create(&tid[i], NULL, proxy_thread, (void*)queue);
    }
    
    addrlen = sizeof(clientaddr);
    while (1)
    {
        if ((connfd = accept(listenfd, (struct sockaddr*)&clientaddr, 
                             &addrlen)) < 0)
        {
            err_exit("accept error");
        }
         
        if ((ret = getnameinfo((struct sockaddr*)&clientaddr, addrlen, 
                        hostname, MAXLINE, port, MAXLINE, 0)))
        {
            fprintf(stderr, "getnameinfo error, code %d\n", ret);
            exit(-1); 
        }

        printf("%s(%d) Connection from %s:%s, fd: %d\n",
                __func__, __LINE__, hostname, port, connfd); 
        queue_push(queue_array[index], (void*)connfd);
        index = (index + 1) % THREAD_NUM;
    }

    close(listenfd);
    return 0;
}

void handle_event(DList* connectionTable, struct epoll_event *ev, int epfd)
{
    int fd = ev->data.fd;
    struct connection* conn = NULL;
    
    if ((conn = find_connection(connectionTable, fd)) == NULL)
    {
        if (ev->events & EPOLLIN)
            get_new_connection(connectionTable, epfd, fd);
        return;
    }

    if ((ev->events & EPOLLIN) && (conn->state == ALL_CONNECTION))
    {
        if (read_from_connection(conn, epfd) != 0)
        {
            struct connection* pair = conn->pair;
            delete_connection(connectionTable, conn);

            if (pair->size == 0)
            {
                /*
                 * If no data needs to forward, we should delete the peer connection
                 */
                char buf[64];

                /*Wait peer closed the connection*/
                shutdown(pair->fd, SHUT_WR);
                while (read(pair->fd, buf, 64) != 0) continue; 
                delete_connection(connectionTable, pair);
            }
            else
            {
                /*
                 * If remain data to forward, we should set the state of pair
                 * HALF_FINISH_CONNECTION, then when send all data, we will delete
                 * the pair connection.
                 */
                conn->pair->state = HALF_FINISH_CONNECTION;
            }
        }
    }
    else if ((ev->events & EPOLLIN) && (conn->state == HALF_CONNECTION))
    {
        if (read_from_half_connection(connectionTable, conn, epfd) != 0)
        {
            delete_connection(connectionTable, conn);
        }
    }
    else if (ev->events & EPOLLOUT)
    {
        struct epoll_event event;

        if (write_to_connection(conn, epfd) < 0)
        {
            /*
             * Set conn->size = 0 just discard the data.
             */
            if (conn->pair)
                shutdown(conn->pair->fd, SHUT_WR);
            conn->size = 0;
        }

        /*
         * If no data needs to send, disable write of the connection
         */
        if (conn->size == 0)
        {
            /* 
             * The pair connection has closed and we have send all data, so
             * delete the connection
             */
            if (conn->state == HALF_FINISH_CONNECTION)
            {
                char buf[64];
                shutdown(conn->fd, SHUT_WR);
                while (read(conn->fd, buf, 64) != 0) continue; 
                delete_connection(connectionTable, conn);
                return;
            }

            memset(&event, 0, sizeof(event));
            event.data.fd = conn->fd;
            event.events = EPOLLIN;
            if (epoll_ctl(epfd, EPOLL_CTL_MOD, conn->fd, &event) == -1)
            {
                fprintf(stderr, "epoll_ctl error\n");
                return;
            }
        }
    }
    else if (ev->events & (EPOLLHUP | EPOLLERR))
    {
        fprintf(stderr, "closing connection: %d\n", conn->fd);
        delete_connection(connectionTable, conn);
    } 
}


void* proxy_thread(void *varg)
{
    int i;
    Queue *thiz = (Queue*)varg;
    DList *connectionTable;
    int epfd;
    int connfd;
    struct epoll_event evlists[MAX_EVENTS];
    int timeout = 10000; //1 second
    int ready;
    
    pthread_detach(pthread_self());

    if ((connectionTable = init_connection_table()) == NULL)
        thread_err_exit("init_connection_table error");

    if ((epfd = epoll_create(MAX_FILENO_PER_THREAD)) != -1)
    {
        while (1)
        {
            struct epoll_event ev;

            /* 
             * Add all the file descriptors in the queue to the "interest
             * list" for the epoll instance
             * 
             */
            while ((connfd = (int)queue_pop(thiz)))
            {
                set_fd_nonblock(connfd);
                if (add_epoll_event(epfd, connfd) == -1)
                    fprintf(stderr, "add_epoll_event failed\n");
            }
            
            ready = epoll_wait(epfd, evlists, MAX_EVENTS, timeout); 
            if (ready == -1) /* Error occured */
            {
                if (errno == EINTR)
                {
                    continue;
                }
                else
                {
                    thread_err_exit("epoll_wait error");
                }
            }
            else if (ready == 0) /* Timeout */
            {
                printf("epoll_wait timeout\n");
                continue;
            }
            else                 /* some events happened */
            {
                for (i = 0; i < ready; i++)
                {
                    handle_event(connectionTable, &evlists[i], epfd);
                }
            }
        }
    }    
    
    deinit_connection_table(connectionTable);

    return NULL;
}
