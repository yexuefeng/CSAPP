/*************************************************************************
	> File Name: test.c
	> Author: ye xuefeng
	> Mail: 
	> Created Time: 2018年02月08日 星期四 11时19分04秒
 ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main()
{
    char *content = calloc(0, 16);

    printf("%d\n", strlen(content));

    return 0;
}
