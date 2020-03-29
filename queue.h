#ifndef QUEUE_H
#define QUEUE_H

typedef struct Node {
    struct Node *next, *prev;
    int val;
} Node;

typedef struct Queue {
    Node* head, *tail;
    // int size;
} Queue;

void initialize_queue(Queue* q, int max_size);
Node* enqueue(Queue* q, int val);
int dequeue(Queue* q);
int peek_head(Queue* q);
int peek_tail(Queue* q);
void move_to_front(Queue* q, Node* n);

void print_list(Queue* q);

#endif /* end of include guard: QUEUE_H */
