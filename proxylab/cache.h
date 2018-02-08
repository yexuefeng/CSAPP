/*************************************************************************
	> File Name: cache.h
	> Author: ye xuefeng
	> Mail: 
	> Created Time: 2018年02月08日 星期四 09时31分56秒
 ************************************************************************/

#ifndef _CACHE_H
#define _CACHE_H

#define MAX_CACHE_SIZE (1024*1024)
#define MAX_OBJECT_SIZE 102400

#define MAX_REQUEST 128

void init_cache(void);
char *search_in_cache(const char *url);
int insert_in_cache(const char *url, const char *content, int len);
#endif
