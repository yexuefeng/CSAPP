/*************************************************************************
	* File Name:    dlist.c
	* Author:       Ye Xuefeng(yexuefeng@163.com)
	* Brief:        double linked list implementation
	* Copyright:    All right resvered by the author
	* Created Time: Wed 11 Jan 2017 09:46:15 AM CST
 ************************************************************************/

#include <stdlib.h>
#include "dlist.h"

struct _DList
{
    DListNode* first;
    void* data_destroy_ctx;
    DataDestroyFunc data_destroy;
};

DList* dlist_create(DataDestroyFunc data_destroy, void* ctx)
{
    DList* thiz = malloc(sizeof(DList));
    if(thiz != NULL)
    {
        thiz->first = NULL;
        thiz->data_destroy_ctx = ctx;
        thiz->data_destroy = data_destroy;
    }

    return thiz;
}

static DListNode* dlist_create_node(DList* thiz, void* data)
{
    DListNode* node = malloc(sizeof(DListNode));
    
    if(node != NULL)
    {
        node->prev = NULL;
        node->next = NULL;
        node->data = data;
    }

    return node;
}

static void dlist_destroy_data(DList* thiz, void* data)
{
    if(thiz->data_destroy != NULL)
    {
        thiz->data_destroy(thiz->data_destroy_ctx, data);
    }
}

static void dlist_destroy_node(DList* thiz, DListNode* node)
{
    if(node != NULL)
    {
        node->prev = NULL;
        node->next = NULL;
        dlist_destroy_data(thiz, node->data);
    }
}

DListNode* dlist_get_node(DList* thiz, size_t index, int fail_ret_last)
{
    DListNode* iter = NULL;
    return_val_if_fail(thiz != NULL, NULL);
    
    iter = thiz->first;
    while(iter != NULL && iter->next != NULL && index > 0)
    {
        iter = iter->next;
        index--;
    }

    if(!fail_ret_last)
    {
        return index > 0 ? NULL : iter;
    }

    return iter;
}

Ret dlist_insert(DList* thiz, size_t index, void* data)
{
    DListNode* node = NULL;
    DListNode* cursor = NULL;
    return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);

    if((node = dlist_create_node(thiz, data)) == NULL)
    {
        return RET_OOM;
    }
    
    if(thiz->first == NULL)
    {
        thiz->first = node;
        return RET_OK;
    }

    cursor = dlist_get_node(thiz, index, 1);
    if(index < dlist_length(thiz))
    {
    
       if(thiz->first == cursor)
       {
           thiz->first = node;
       }
       else
       {
           cursor->prev->next = node;
           node->prev = cursor->prev;
       }
       node->next = cursor;
       cursor->prev = node;
    }
    else
    {
        node->prev = cursor;
        cursor->next = node;
    }
    
    return RET_OK;
}

Ret dlist_prepend(DList* thiz, void* data)
{
    return dlist_insert(thiz, 0, data);
}

Ret dlist_append(DList* thiz, void* data)
{
    return dlist_insert(thiz, -1, data);
}

void *dlist_delete(DList* thiz, size_t index)
{
    DListNode* cursor = NULL;
    return_val_if_fail(thiz != NULL, NULL);

    cursor = dlist_get_node(thiz, index, 0);
    if(cursor != NULL)
    {
        if(cursor == thiz->first)
        {
            thiz->first = cursor->next;
        }

        if(cursor->prev != NULL)
        {
            cursor->prev->next = cursor->next;
        }
        if(cursor->next != NULL)
        {
            cursor->next->prev = cursor->prev;
        }

        dlist_destroy_node(thiz, cursor);
    }

    return cursor ? cursor->data : NULL;
}

size_t dlist_length(DList* thiz)
{
    size_t length = 0;
    return_val_if_fail(thiz != NULL, 0);

    DListNode* iter = thiz->first;
    while(iter != NULL)
    {
        length++;
        iter = iter->next;
    }

    return length;
}

Ret dlist_get_by_index(DList* thiz, size_t index, void** data)
{
    DListNode* cursor = NULL;
    return_val_if_fail(thiz != NULL && data != NULL, RET_INVALID_PARAMS);
    
    cursor = dlist_get_node(thiz, index, 0);
    if(cursor != NULL)
    {
        *data = cursor->data;
    }

    return cursor != NULL ? RET_OK : RET_FAIL;
}

Ret dlist_set_by_index(DList* thiz, size_t index, void* data)
{
    DListNode* cursor = NULL;
    return_val_if_fail(thiz != NULL, RET_INVALID_PARAMS);

    cursor = dlist_get_node(thiz, index, 0);
    if(cursor != NULL)
    {
        cursor->data = data;
    }

    return cursor != NULL ? RET_OK : RET_FAIL;
}

Ret dlist_foreach(DList* thiz, DataVisitFunc visit, void* ctx)
{
    Ret ret = RET_OK;
    DListNode* iter = NULL;
    return_val_if_fail(thiz != NULL && visit != NULL, RET_INVALID_PARAMS);
    
    iter = thiz->first;
    while(iter != NULL && ret == RET_OK)
    {
        ret = visit(ctx, iter->data);
        iter = iter->next;
    }

    return ret;
}

int dlist_find(DList* thiz, DataCompareFunc cmp, void* ctx)
{
    int i = 0;
    DListNode* iter = NULL;
    return_val_if_fail(thiz != NULL && cmp != NULL, RET_INVALID_PARAMS);

    iter = thiz->first;
    while(iter != NULL)
    {
        if(cmp(iter, ctx) == 0)
        {
            break;
        }
        i++;
        iter = iter->next;
    }

    return iter != NULL ? i : -1;
}

void dlist_destroy(DList* thiz)
{
    DListNode* iter = NULL;
    DListNode* next = NULL;

    return_if_fail(thiz != NULL);

    iter = thiz->first;
    while(iter != NULL)
    {
        next = iter->next;
        dlist_destroy_node(thiz, iter);
        iter = next;
    }

    thiz->first = NULL;
    free(thiz);

    return;
}

#ifdef DLIST_TEST

#include <assert.h>
#include <stdio.h>

static Ret int_print(void* ctx, void* data)
{
    printf("%d ", (int)data);

    return RET_OK;
}

static void dlist_init_test(void)
{
    size_t i = 0;
    size_t n = 100;
    int data = 0;
    DList* thiz = dlist_create(NULL, NULL);

    for(i = 0; i < n; i++)
    {
        assert(dlist_append(thiz, (void*)i) == RET_OK);
        assert(dlist_length(thiz) == (i+1));
        assert(dlist_get_by_index(thiz, i, (void**)&data) == RET_OK);
        assert(data == i);
        assert(dlist_set_by_index(thiz, i, (void*)(2*i)) == RET_OK);
        assert(dlist_get_by_index(thiz, i, (void**)&data) == RET_OK);
        assert(data == (2*i));
        assert(dlist_set_by_index(thiz, i, (void*)i) == RET_OK);
    }

    assert(dlist_foreach(thiz, int_print, NULL) == RET_OK);

    for(i = 0; i < n; i++)
    {
        assert(dlist_length(thiz) == (n - i));
        assert(dlist_delete(thiz, 0) == RET_OK);
        assert(dlist_length(thiz) == (n - i - 1));
        if(i < (n - 1))
        {
            assert(dlist_get_by_index(thiz, 0, (void**)&data) == RET_OK);
            assert(data == (i+1));
        }
    }


    for(i = 0; i < n; i++)
    {
        assert(dlist_prepend(thiz, (void*)i) == RET_OK);
        assert(dlist_length(thiz) == (i + 1));
        assert(dlist_get_by_index(thiz, 0, (void**)&data) == RET_OK);
        assert(data == i);
        assert(dlist_set_by_index(thiz, 0, (void*)(2*i)) == RET_OK);
        assert(dlist_get_by_index(thiz, 0, (void**)&data) == RET_OK);
        assert(data == (2*i));
        assert(dlist_set_by_index(thiz, 0, (void*)i) == RET_OK);
    }

    assert(dlist_foreach(thiz, int_print, NULL) == RET_OK);

    dlist_destroy(thiz);

    return;
}

static void dlist_params_test(void)
{
    printf("===============normal Warning start=================\n");
    assert(dlist_insert(NULL, 0, NULL) == RET_INVALID_PARAMS);
    assert(dlist_prepend(NULL, NULL) == RET_INVALID_PARAMS);
    assert(dlist_append(NULL, NULL) == RET_INVALID_PARAMS);
    assert(dlist_delete(NULL, 0) == RET_INVALID_PARAMS);
    assert(dlist_get_by_index(NULL, 0, NULL) == RET_INVALID_PARAMS);
    assert(dlist_set_by_index(NULL, 0, NULL) == RET_INVALID_PARAMS);
    assert(dlist_foreach(NULL, NULL, NULL) == RET_INVALID_PARAMS);
    assert(dlist_find(NULL, NULL, NULL) == RET_INVALID_PARAMS);
    printf("===============normal Warning end===================\n");

    return;
}

int main(int argc, char* argv[])
{
    dlist_init_test();
    dlist_params_test();

    return 0;
}
#endif
