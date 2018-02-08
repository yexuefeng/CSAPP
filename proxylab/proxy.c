#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>

#include "csapp.h"
#include "robust_io.h"
#include "sbuf.h"
#include "cache.h"

/* Recommended max cache and object sizes */

#define THREAD_NUM 4

pthread_t tid[THREAD_NUM];
extern sbuf_t sbuffer;

void* proxy_thread(void *argv);

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";


/*
 * parse_url - Parse the url in the http request. Return 0 if success, or -1
 *             if the request is malformed or malicious.
 */
int parse_url(const char *request, char *hostname, char *uri, char *port)
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

static void handle_get(int connfd,
                       char *url,
                       char *method,
                       char *hostname,
                       char *uri,
                       char *port)
{
    int clientfd;
    char request[MAXLINE];
    char response[MAXLINE];
    char *content;
    ssize_t nread;
    ssize_t nwrite;
    int size;
    
    /*First we should search in the cache*/
    if ((content = search_in_cache(url)))
    {
        printf("%s(%d) Get content from cache\n", __func__, __LINE__);
        nwrite = ro_write(connfd, content, strlen(content));
        if (nwrite != strlen(content))
        {
            printf("%s(%d) Warning short count in write process\n",
                   __func__, __LINE__);
        }
        
        free(content);
        return;
    } 
    /*
     * If the request url not found in cache, then we should forward the request to the 
     * remote web server.
     */
    if ((clientfd = open_clientfd(hostname, port)) < 0)
    {
        printf("open_clientfd error");
        return;
    }
   
    make_request_packet(request, method, hostname, uri, port);
    
    nwrite = ro_write(clientfd, request, strlen(request));

    /* Wait for response */
    memset(&response, 0, sizeof(response));
    size = 0;
    if ((content = calloc(MAX_OBJECT_SIZE, 1)) == NULL)
    {
        close(clientfd);
        return;
    }

    while (1)
    {
        nread = read(clientfd, response, MAXLINE);
        if (nread > 0)
        {
            ro_write(connfd, response, nread);
            size += nread;
            if (size <= MAX_OBJECT_SIZE)
            {
                strncpy(content + size - nread, response, nread);
            }
        }
        else if (nread < 0)
        {
            if (errno == EINTR)
                continue;
            else            /* Error occured */
                break;
        }
        else  /* EOF, means peer client has closed this connection */
            break;
    }
    
    if (size > 0 && size <= MAX_OBJECT_SIZE) 
        insert_in_cache(url, content, size);

    /* Safely closed the connection */
    free(content);
    close(clientfd);
}

/*
 * receive_request_from_client - Receive http request from client.
 */
void receive_request_from_client(int connfd)
{
    char method[MAXLINE], version[MAXLINE], url[MAXLINE];
    char request[MAXLINE]; 
    char hostname[MAXLINE], uri[MAXLINE], port[MAXLINE];
    ssize_t nread;
    int flags;
    
    fcntl(connfd, F_GETFL, &flags);
    flags |= O_NONBLOCK; 
    fcntl(connfd, F_SETFL, O_NONBLOCK);

    while ((nread = ro_read(connfd, request, MAXLINE)) >= 0)
    {
        if (nread > 0)
        {
            sscanf(request, "%s %s %s", method, url, version);
            if (parse_url(url, hostname, uri, port) == -1)
            {
                /* Parse url failed*/
                break;
            }
            
            if (!strcasecmp(method, "GET"))
            {
                handle_get(connfd, url, "GET", hostname, uri, port);
                break;
            }
        }
    }
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
    int ret;

    if (argc < 2)
    {
        display_usage(argv[0]);
    }
    
    if ((listenfd = open_listenfd(argv[1])) < 0)
    {
        err_exit("open_listenfd error");
    } 
    set_socket_reuse(listenfd);

    sbuf_init(&sbuffer, 64);
    init_cache();
    signal(SIGPIPE, SIG_IGN);

    /* Create thread pool */ 
    for (i = 0; i < THREAD_NUM; i++)
    {
        pthread_create(&tid[i], NULL, proxy_thread, NULL);
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
        printf("%s(%d) Connection from %s:%s\n",
                __func__, __LINE__, hostname, port);
        sbuf_insert(&sbuffer, connfd);
    }

    close(listenfd);
    sbuf_deinit(&sbuffer);
    return 0;
}

void* proxy_thread(void *varg)
{
    pthread_detach(pthread_self());
    int i;

    while (1)
    {
        int connfd = sbuf_remove(&sbuffer);
        for (i = 0; i < THREAD_NUM; i++)
        {
            if (pthread_equal(tid[i], pthread_self()))
                printf("Pthread %d handle connfd %d\n", i, connfd);
        }
        receive_request_from_client(connfd);
        close(connfd);
    }    
}
