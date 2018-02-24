/*************************************************************************
	* File Name:    queue.h
	* Author:       Ye Xuefeng(yexuefeng@163.com)
	* Copyright:    All right resvered by the author
	* Created Time: Mon 09 Jan 2017 02:18:53 PM CST
 ************************************************************************/

#ifndef _QUEUE_H
#define _QUEUE_H

#include "dlist.h"

struct _Queue;
typedef struct _Queue Queue;


Queue*    queue_create(DataDestroyFunc data_destroy, void* ctx);
Ret       queue_head(Queue* thiz, void** data);
Ret       queue_push(Queue* thiz, void* data);
void      *queue_pop(Queue* thiz);
size_t    queue_length(Queue* thiz);
Ret       queue_foreach(Queue* thiz, DataVisitFunc visit, void* ctx);
void      queue_destroy(Queue* thiz);

#endif
