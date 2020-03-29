#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

void initialize_queue(Queue* q, int max_size) {
    q->head = NULL;
    q->tail = NULL;
    // q->size = 0;
}



Node* enqueue(Queue* q, int val) {
    Node* new = (Node*)malloc(sizeof(Node));
    new->val = val;
    new->next=q->head;

    if(!q->head) {
        q->head = new;
        q->tail = new;
    } else {
        q->head->prev = new;
        q->head = new;
    }
    // ++(q->size);
    return new;
}



int dequeue(Queue* q) {
    if(!q->head) {
        return -1;
    } else {
        int retval = q->tail->val; // storing the value to be dequeued
        Node* old = q->tail;       // old node to be deleted
        q->tail = q->tail->next;
        q->tail->prev = NULL;
        free(old);
        return retval;
    }
}



int peek_head(Queue* q) {
    return q->head->val;
}



int peek_tail(Queue* q) {
    return q->tail->val;
}


void move_to_front(Queue* q, Node* n) {
    if(!q->head) return;

    if(n != q->head) {
        // unlinking node
        if(n->next)
            n->next->prev = n->prev;
        if(n->prev)
            n->prev->next = n->next;

        // if the node is the tail
        if(n == q->tail){
            q->tail = n->prev;
            q->tail->next = NULL;
        }

        // putting to front
        n->next = q->head;
        n->prev = NULL;
        q->head->prev = n;
        q->head = n;
    }
}



void print_list(Queue* list) {
    if(!list->head) { printf("List empty."); }
    else { printf("List: "); }
    Node *tmp = list->head;
    while(tmp) {
        printf("%d ", tmp->val);
        tmp = tmp->next;
    }
    printf("\n");
}
