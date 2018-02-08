#ifndef _SBUF_H
#define _SBUF_H

#include <semaphore.h>

struct sbuf;
typedef struct sbuf sbuf_t;

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

#endif
