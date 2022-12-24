//Create Periority queue of processes
//original source: http://rosettacode.org/wiki/Priority_queue#C

#ifndef OS_STARTER_CODE_PROCESSHEAP_H
#define OS_STARTER_CODE_PROCESSHEAP_H

#include <stdio.h>
#include <stdlib.h>
#include "Process_Struct.h"

typedef struct {
    int priority;
    Process* data;
} node_t;

typedef struct {
    node_t *nodes;
    int len;
    int size;
} heap_t;

void HeapPush(heap_t *h, int priority, Process* data) {
    if (h->len + 1 >= h->size) {
        h->size = h->size ? h->size * 2 : 4;
        h->nodes = (node_t *) realloc(h->nodes, h->size * sizeof(node_t));
    }
    int i = h->len + 1;
    int j = i / 2;
    while (i > 1 && h->nodes[j].priority > priority) {
        h->nodes[i] = h->nodes[j];
        i = j;
        j = j / 2;
    }
    h->nodes[i].priority = priority;
    h->nodes[i].data = data;
    h->len++;
}

Process* HeapPop(heap_t *h) {
    int i, j, k;
    if (!h->len) {
        return NULL;
    }
    Process* data = h->nodes[1].data;

    h->nodes[1] = h->nodes[h->len];

    h->len--;

    i = 1;
    while (i != h->len + 1) {
        k = h->len + 1;
        j = 2 * i;
        if (j <= h->len && h->nodes[j].priority < h->nodes[k].priority) {
            k = j;
        }
        if (j + 1 <= h->len && h->nodes[j + 1].priority < h->nodes[k].priority) {
            k = j + 1;
        }
        h->nodes[i] = h->nodes[k];
        i = k;
    }
    return data;
}

Process* HeapPeek(heap_t *h) {
    if (!h->len) {
        return NULL;
    }

    return h->nodes[1].data;
}

int HeapEmpty(heap_t *h) {
    return h->len == 0;
}

#endif //OS_STARTER_CODE_PROCESSHEAP_H
