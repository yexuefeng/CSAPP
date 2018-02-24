/*************************************************************************
	* File Name:    dlist.h
	* Author:       Ye Xuefeng(yexuefeng@163.com)
	* Brief:        dlist header file
	* Copyright:    All right resvered by the author
	* Created Time: Wed 11 Jan 2017 09:38:56 AM CST
 ************************************************************************/
#ifndef _DLIST_H
#define _DLIST_H

#include "typedef.h"

typedef struct _DListNode {
    struct _DListNode *prev;
    struct _DListNode *next;
    void *data;
}DListNode;

struct _DList;
typedef struct _DList DList;

DList* dlist_create(DataDestroyFunc data_destroy, void* ctx);


Ret    dlist_insert(DList* thiz, size_t index, void* data);
Ret    dlist_prepend(DList* thiz, void* data);
Ret    dlist_append(DList* thiz, void* data);
void   *dlist_delete(DList* thiz, size_t index);
size_t dlist_length(DList* thiz);
Ret    dlist_get_by_index(DList* thiz, size_t index, void** data);
Ret    dlist_set_by_index(DList* thiz, size_t index, void* data);
Ret    dlist_foreach(DList* thiz, DataVisitFunc visit, void* ctx);
int    dlist_find(DList* thiz, DataCompareFunc cmp, void* ctx);
DListNode* dlist_get_node(DList* thiz, size_t index, int fail_ret_last);

void   dlist_destroy(DList* thiz);

#endif

