/***********************************************\
*                   FIFOqueue.h                 *
*           George Koskeridis (C) 2016          *
\***********************************************/

#ifndef __FIFO_QUEUE_H
#define __FIFO_QUEUE_H

#include <stdbool.h>

#define MALLOC        0x000a
#define WIN_MALLOC    0x000b
#define AUTO          0x000c

typedef struct _FIFOnode {
    void *data;
    struct _FIFOnode *next;
} FIFOnode;

typedef struct _FIFOqueue {
    FIFOnode *head;
    FIFOnode *tail;
    unsigned int total_nodes;
} FIFOqueue;


FIFOqueue *newFIFOqueue(void);
bool FIFOenqueue(FIFOqueue *queue, void *node_data);
void *FIFOdequeue(FIFOqueue *queue);
bool deleteFIFOqueue(FIFOqueue *queue, int flag);

#endif //__FIFO_QUEUE_H
