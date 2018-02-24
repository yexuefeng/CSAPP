/*************************************************************************
	* File Name:    queue.c
	* Author:       Ye Xuefeng(yexuefeng@163.com)
	* Brief:        queue implementation 
	* Copyright:    All right resvered by the author
	* Created Time: Mon 09 Jan 2017 02:26:43 PM CST
 ************************************************************************/
#include "queue.h"

#include <stdlib.h>
#include <pthread.h>

#include "dlist.h"

struct _Queue
{
    DList* dlist;
    pthread_mutex_t mutex;
};

pthread_mutex_t mutex;

Queue* queue_create(DataDestroyFunc data_destroy, void* ctx)
{
    Queue* thiz = malloc(sizeof(Queue));
    pthread_mutex_init(&thiz->mutex, NULL);
    if(thiz != NULL)
    {
        thiz->dlist = dlist_create(data_destroy, ctx);
        if(thiz->dlist == NULL)
        {
            free(thiz);
            thiz = NULL;
        }
    }

    return thiz; 
}

Ret queue_head(Queue* thiz, void** data)
{
    Ret ret;

    return_val_if_fail(thiz != NULL && data != NULL, RET_INVALID_PARAMS); 
    pthread_mutex_lock(&thiz->mutex);
    ret = dlist_get_by_index(thiz->dlist, 0, data);
    pthread_mutex_unlock(&thiz->mutex);
    
    return ret;
}

Ret queue_push(Queue* thiz, void* data)
{
    Ret ret;

    return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);
    pthread_mutex_lock(&thiz->mutex);
    ret = dlist_append(thiz->dlist, data);
    pthread_mutex_unlock(&thiz->mutex);

    return ret;
}

void* queue_pop(Queue* thiz)
{
    void* ret;

    return_val_if_fail(thiz != NULL, NULL);
    pthread_mutex_lock(&thiz->mutex);
    ret = dlist_delete(thiz->dlist, 0);
    pthread_mutex_unlock(&thiz->mutex);
    return ret;
}

size_t queue_length(Queue* thiz)
{
    size_t len;
    return_val_if_fail(thiz != NULL, 0);
    pthread_mutex_lock(&thiz->mutex);
    len = dlist_length(thiz->dlist);
    pthread_mutex_unlock(&thiz->mutex);
    return len;
}

Ret queue_foreach(Queue* thiz, DataVisitFunc visit, void* ctx)
{
    Ret ret;

    return_val_if_fail(thiz != NULL && visit != NULL, RET_INVALID_PARAMS);
    pthread_mutex_lock(&thiz->mutex);
    ret = dlist_foreach(thiz->dlist, visit, ctx);
    pthread_mutex_unlock(&thiz->mutex);

    return ret;
}

void queue_destroy(Queue* thiz)
{
    if(thiz != NULL)
    {
        dlist_destroy(thiz->dlist);
        thiz->dlist = NULL;
        pthread_mutex_destroy(&thiz->mutex);
        SAFE_FREE(thiz);
    }
    return;
}

#ifdef QUEUE_TEST

#include <assert.h>
#include <stdio.h>

static Ret int_print(void* ctx, void* data)
{
    printf("%d ", (int)data);
    return DLIST_RET_OK;
}

static int queue_init_test(void)
{
    int i = 0;
    int n = 100;
    int data = 0;
    Queue* thiz = queue_create(NULL, NULL);

    for(i = 0; i < n; i++)
    {
        assert(queue_push(thiz, (void*)i) == DLIST_RET_OK);
        assert(queue_length(thiz) == (i+1));
    }

    assert(queue_foreach(thiz, int_print, 0) == DLIST_RET_OK);

    for(i = 0; i < n; i++)
    {
        assert(queue_head(thiz, (void**)&data) == DLIST_RET_OK);
        assert(data == i);
        assert(queue_length(thiz) == (n - i));
        assert(queue_pop(thiz) == DLIST_RET_OK);
        assert(queue_length(thiz) == (n - i -1));
    }

    queue_destroy(thiz);
}

static void queue_params_test(void)
{
    printf("================normal Warning start====================\n");
    assert(queue_push(NULL, NULL) == RET_INVALID_PARAMS);
    assert(queue_head(NULL, NULL) == RET_INVALID_PARAMS);
    assert(queue_pop(NULL) == RET_INVALID_PARAMS);
    assert(queue_length(NULL) == 0);
    assert(queue_foreach(NULL, NULL, NULL) == RET_INVALID_PARAMS);
    printf("================normal Warning start====================\n");
}

int main(int argc, char* argv[])
{
    queue_init_test();
    queue_params_test();
    return 0;
}

#endif
