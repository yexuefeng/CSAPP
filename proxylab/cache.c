/*************************************************************************
	> File Name: cache.c
	> Author: ye xuefeng
	> Mail: 
	> Created Time: 2018年02月07日 星期三 14时53分46秒
 ************************************************************************/
#include "cache.h"

#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>

struct object
{
    char key[MAX_REQUEST]; // key of the object, always the request string.
    char *data; // Content of the object
    int size;  // data size in byte
    struct timeval ctime; //Update time
    struct object *next;
};

struct objecthead {
    struct object *first; // Point the first item in the cache object list.
    struct object *last;
    int size; // The total size of objects int the list.
    int count; // the counts of the objects number
    pthread_mutex_t mtx;
} head;

/*
 * init_cache - Initialize the cache database
 */
void init_cache(void)
{
    head.first = NULL;
    head.last = NULL;
    head.size = 0;
    head.count = 0;
    pthread_mutex_init(&head.mtx, NULL);
}

/*
 * search_in_cache - Search a specified object in cache database.
 * Return the cache object pointer if found, or NULL if not found
 */
char *search_in_cache(const char *url)
{
    struct object *current;
    char *ptr;
    
    pthread_mutex_lock(&head.mtx);
    current = head.first;

    while (current)
    {
        if (!strcmp(current->key, url))
        {
            /* Update time */
            gettimeofday(&(current->ctime), NULL);
            ptr = (char*)malloc(current->size + 1);
            if (ptr)
            {
                strncpy(ptr, current->data, current->size);
                ptr[current->size] = '\0';
            }
            pthread_mutex_unlock(&head.mtx);
            return ptr;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&head.mtx);
    return NULL;
}

/*
 * Little helper function. If t1 > t2, return 1, else return 0
 */
static int compare_time(struct timeval t1, struct timeval t2)
{
    if (t1.tv_sec > t2.tv_sec)
        return 1;
    else if ((t1.tv_sec == t2.tv_sec) && (t1.tv_usec >= t2.tv_usec))
        return 1;
    else
        return 0;
}

static char *search_in_cache_without_lock(const char *url)
{
    struct object *current;
    char *ptr;
    
    current = head.first;

    while (current)
    {
        if (!strcmp(current->key, url))
        {
            /* Update time */
            gettimeofday(&(current->ctime), NULL);
            ptr = (char*)malloc(current->size + 1);
            if (ptr)
            {
                strncpy(ptr, current->data, current->size);
                ptr[current->size] = '\0';
            }
            return ptr;
        }
        current = current->next;
    }

    return NULL;
}

/*
 * evict_object - Evict object based on least-recently-used (LRU) policy.
 */
static void evict_object(void)
{
    struct object *current, *prev;
    struct object *prev_eviction, *eviction;
    
    /*
     * Caution, the cache database has locked in function insert_in_cache
     */
    current = head.first;
    prev = NULL;
    eviction = head.first;
    prev_eviction = NULL;

    /* Find the least-recently-used object */
    while (current)
    {
        if (compare_time(eviction->ctime, current->ctime))
        {
            eviction = current;
            prev_eviction = prev;
        }
        
        /* Update */
        prev = current;
        current = current->next;
    }

    /* Delete the least recently used object */
    if (eviction == head.first)
        head.first = eviction->next;
    if (eviction == head.last)
        head.last = prev_eviction;
    
    if (prev_eviction)
       prev_eviction->next = eviction->next; 
    head.count--;
    head.size -= eviction->size;
    free(eviction->data);
    free(eviction);
}

/*
 * insert_in_cache - Insert a new object in the cache database
 * Return 0 if success, or -1 if failed.
 */
int insert_in_cache(const char *url, const char *content, int len)
{
    struct object *p;
    struct object *last = NULL;
    int url_len = strlen(url);
    
    if (len > MAX_OBJECT_SIZE || len <= 0 || url_len >= MAX_REQUEST)
        return -1;
    
    /* Make an new object */
    p = (struct object*)malloc(sizeof(struct object));
    if (p == NULL)
        return -1;
    p->data = (char*)malloc(len);
    if (p->data == NULL)
    {
        free(p);
        return -1;
    }
    strncpy(p->data, content, len);
    strncpy(p->key, url, url_len);
    p->key[url_len] = '\0';
    p->size = len; 
    p->next = NULL;    
    gettimeofday(&(p->ctime), NULL);

    /* Critical section */
    pthread_mutex_lock(&head.mtx);
    while ((len + head.size) > MAX_CACHE_SIZE)
        evict_object();
    /* To avoid insert an exist item */
    if (!search_in_cache_without_lock(url))
    {
        last = head.last;
        if (!head.first)
            head.first = p;
        if (last)
            last->next = p;
        head.last = p;
        head.size += len;
        head.count++;
        pthread_mutex_unlock(&head.mtx);

        return 0;
    }
    
        pthread_mutex_unlock(&head.mtx);
    return -1;
}

/*
 * get_object_content - return the pointer of object data 
 */
char* get_object_content(void *obj)
{
    return ((struct object*)obj)->data;
}

/*
 * get_object_size - return the size of object content
 */
int get_object_size(void *obj)
{
    return ((struct object*)obj)->size;
}

/*
 * update_object_age - Update the object aging time
 */
void update_object_age(void *obj, struct timeval time)
{
    ((struct object*)obj)->ctime = time;
}

#ifdef CACHE_TEST

#include <assert.h>

void* test_insert_single_thread(void *arg)
{
    int i;
    int size = MAX_OBJECT_SIZE;
    char url[64] = "http://www.baidu.com";
    char *content;
    struct object *ptr;
    
    /* 1. test whether it inserts an exist item? */  
    content = malloc(size);
    assert(0 == insert_in_cache(url, content, size));
    assert(head.count == 1);
    assert(head.size == size);

    content = malloc(size);
    assert(-1 == insert_in_cache(url, content, size));

    /* 2. test the cache size */
    for (i = 0; i < 10; i++)
    {
        sprintf(url, "www.biadu.com%d", i);
        content = malloc(size); 
        if (i < 9)
        {
            assert(0 == insert_in_cache(url, content, size));
            assert(head.count == i + 2);
            assert(head.size == (i+2)*MAX_OBJECT_SIZE);
        }
        
        /* Eviction occurred */
        if (i >= 9)
        {
            insert_in_cache(url, content, size);
            assert(head.count == 10);
            assert(head.size == 10*MAX_OBJECT_SIZE); 
        }
    }
    ptr = head.first; 
    while (ptr)
    {
        printf("%p: %s, %d\n", ptr, ptr->key, ptr->size);
        ptr = ptr->next;
    }

    return NULL;
}

void* thread_func(void *argv)
{
    char url[] = "http://www.hao123.com";
    int size = MAX_OBJECT_SIZE;
    char *content = (char*)malloc(size);
    struct object *first;

    insert_in_cache(url, content, size);
    
    pthread_mutex_lock(&head.mtx);
    assert(head.count == 1);
    assert(head.size == MAX_OBJECT_SIZE);
    first = head.first;
    printf("Elapsed %lu seconds, %lu microseconds\n",
            first->ctime.tv_sec, first->ctime.tv_usec);
    pthread_mutex_unlock(&head.mtx);

    return NULL;
}

void test_insert_multi_thread(void)
{
    int threadNum = 4;
    int i;
    pthread_t tid[4];

    for (i = 0; i < threadNum; i++)
    {
        pthread_create(&tid[i], NULL, thread_func, NULL);
    }
    
    for (i = 0; i < threadNum; i++)
       pthread_join(tid[i], NULL); 

}

void* test_search(void *argv)
{
    char url1[64] = "http://www.baidu.com";
    char url2[64] = "http://www.alibaba.com";
    char url3[64] = "http://www.tecent.com";
    char *ptr;

    ptr = malloc(64);
    strncpy(ptr, url1, 64);
    assert(0 == insert_in_cache(url1, ptr, strlen(url1) + 1));
    free(ptr);
    ptr = search_in_cache(url1);
    assert(strcmp(ptr, url1) == 0);
    free(ptr);
    
    ptr = malloc(64);
    strncpy(ptr, url2, 64);
    assert(0 == insert_in_cache(url2, ptr, strlen(url2) + 1));
    free(ptr);
    ptr = search_in_cache(url2);
    assert(strcmp(ptr, url2) == 0);
    free(ptr);
    
    ptr = malloc(64);
    strncpy(ptr, url3, 64);
    assert(0 == insert_in_cache(url3, ptr, strlen(url3) + 1));
    free(ptr);
    ptr = search_in_cache(url3);
    assert(strcmp(ptr, url3) == 0);
    free(ptr);

    return NULL;
}

int main()
{
    init_cache();
    //test_insert_single_thread(NULL);
    test_insert_multi_thread();
    test_search(NULL);
    return 0;
}
#endif 

