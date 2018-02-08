#include "sbuf.h"

#include <semaphore.h>
#include <stdlib.h>

#include "csapp.h"

struct sbuf {
    int *buf;    /* Buffer array */
    int n;       /* Maximum number of slots */
    int front;   /* buf[(front + 1) % n] is the first item */
    int rear;    /* buf[rear%n] is last item */
    sem_t mutex; /* Protects access to buf */
    sem_t slots; /* Counts available slots */
    sem_t items; /* Counts available items */
};

sbuf_t sbuffer;
/* Create an empty, bounded shared FIFO buffer with slots */
void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = calloc(n, sizeof(int));
    if (!sp->buf)
    {
        err_exit("calloc error");
    }
    sp->n = n;
    sp->front = sp->rear = 0;
    sem_init(&sp->mutex, 0, 1);
    sem_init(&sp->slots, 0, n);
    sem_init(&sp->items, 0, 0);
}

/* Clean up buffer sp */
void sbuf_deinit(sbuf_t *sp)
{
    free(sp->buf);
}

/* Insert item onto the rear of shared buffer sp */
void sbuf_insert(sbuf_t *sp, int item)
{
    sem_wait(&sp->slots);   /* Wait for available slot */
    sem_wait(&sp->mutex);   /* Lock the buffer */
    sp->buf[(++sp->rear)%sp->n] = item;  /* Insert the item */
    sem_post(&sp->items);   /* Announce available item */
    sem_post(&sp->mutex);   /* Unlock the buffer */
}

/* Remove and return the first item from buffer sp */
int sbuf_remove(sbuf_t *sp)
{
    int item;
    sem_wait(&sp->items);
    sem_wait(&sp->mutex);
    item = sp->buf[(++sp->front)%(sp->n)];
    sem_post(&sp->slots);
    sem_post(&sp->mutex);
    return item;
}

