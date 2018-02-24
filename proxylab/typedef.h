/*************************************************************************
	* File Name:    typedef.h
	* Author:       Ye Xuefeng(yexuefeng@163.com)
	* Brief:        
	* Copyright:    All right resvered by the author
	* Created Time: Wed 11 Jan 2017 09:12:35 AM CST
 ************************************************************************/
#ifndef _TYPEDEF_H
#define _TYPEDEF_H

#include <stdio.h>
#include <stdlib.h>

typedef enum _Ret
{
    RET_OK,
    RET_OOM,
    RET_STOP,
    RET_INVALID_PARAMS,
    RET_FAIL
}Ret;

typedef void   (*DataDestroyFunc)(void* ctx, void* data);
typedef Ret    (*DataVisitFunc)(void* ctx, void* data);
typedef int    (*DataCompareFunc)(void* ctx, void* data);

#define return_if_fail(p) if(!(p))   \
    {printf("%s:%d Warning #p!\n",   \
            __func__, __LINE__); return;}
#define return_val_if_fail(p, ret) if(!(p)) \
    {printf("%s:%d Warning #p!\n",   \
            __func__, __LINE__); return(ret);}

#define SAFE_FREE(p) if(!(p)) {free(p); p = NULL;}

#endif

