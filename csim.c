#include "cachelab.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

// Parameters
int verbose = 0;
int set_bits = 0;
int assoc = 1;
int block_bits = 0;
FILE* trace_file;

// Results
int hits = 0;
int misses = 0;
int evictions = 0;
int dirty_evicted = 0;
int dirty_active = 0;
int double_accesses = 0;

/**************** Helper Functions ********************************/

/* Queue Node */
typedef struct Node {
   struct Node *next, *prev;
   int val;
} Node;

/* Queue struct */
typedef struct Queue {
   Node* head, *tail;
   // int size;
} Queue;

/* Queue Methods */
void initialize_queue(Queue* q, int max_size);
Node* enqueue(Queue* q, int val);
int dequeue(Queue* q);
int peek_head(Queue* q);
int peek_tail(Queue* q);
void move_front(Queue* q, Node* n);

void print_list(Queue* q);

/*
* get_opt_args - This function reads and sets the input arguments of the
* simulator.
*/
void get_opt_args(int argc, char* argv[]);

/* Prints only when verbose is true*/
void verbose_print(char* str);

/* Performs the necessary actions for a data load */
void data_load(int set_index, int line_index, long tag, int miss, long* tags,
            char* valid, char* dirty, Queue* usage_queue, Node** usage_table);

/* Performs the necessary actions for a data store */
void data_store(int set_index, int line_index, long tag, int miss, long* tags,
            char* valid, char* dirty, Queue* usage_queue, Node** usage_table);

/* Updates global variables for a cache eviction */
void cache_eviction(int line, char* dirty);

/* Updates global variables for a cache hit*/
void cache_hit(int line_index, int set_index, Queue* usage_queue);


/*************************** Code ********************************/

int main(int argc, char* argv[])
{
   int i;
   get_opt_args(argc, argv);

   // masks to access certain parts of address
   unsigned long block_mask = (1 << block_bits)-1;
   unsigned long set_mask = ((1 << set_bits)-1) << block_bits;

   // allocating memory for cache
   long* tags = (long*)malloc((1 << set_bits)*assoc*sizeof(long));
   char* valid = (char*)malloc((1 << set_bits)*assoc*sizeof(char));
   char* dirty = (char*)malloc((1 << set_bits)*assoc*sizeof(char));

   // for tracking usage
   Queue* usage_queue = (Queue*)malloc((1 << set_bits)*sizeof(Queue));
   // Hash table for quick access
   Node** usage_table = (Node**)malloc((1 << set_bits)*assoc*sizeof(Node*));

   // reading trace file
   char type;
   unsigned long tr_addr;
   int num_bytes;
   while(fscanf(trace_file, " %c %lx,%i", &type, &tr_addr, &num_bytes) == 3) {
      int line_index;
      int cold_index;
      int miss;
      if(verbose) {
         printf("%c %lx,%i ", type, tr_addr, num_bytes);
      }
      unsigned int set_index = (tr_addr & set_mask) >> block_bits;
      // int block_index = (tr_addr & block_mask);
      unsigned long tag;
      tag = (tr_addr & ~(set_mask | block_mask)) >> (set_bits+block_bits);

      if(type != 'I') {
         // finding if there is a cache hit or miss
         line_index = -1;
         cold_index = -1;
         for(i=0; i<assoc; ++i) {
            // cache hit (should only run once)
            if(tag == tags[set_index*assoc+i] && valid[set_index*assoc+i]){
               line_index=i;
            }

            // storing the first unfilled line
            if(!valid[set_index*assoc+i] && cold_index == -1) {
               cold_index = i;
            }
         }

         // finding the appropriate line to write to
         if(line_index == -1 && cold_index != -1) { // cold miss
            line_index = cold_index;
            miss = 2;
         } else if(line_index == -1) { // miss
            line_index = peek_tail(&usage_queue[set_index]);
            miss = 1;
         } else { // hit
            miss = 0;
         }

         // performing appropriate actions based on the action
         switch(type) {
            case 'L':
            data_load(set_index, line_index, tag, miss, tags, valid,
                      dirty, usage_queue, usage_table);
            break;

            case 'S':
            data_store(set_index, line_index, tag, miss, tags, valid,
                       dirty, usage_queue, usage_table);
            break;

            case 'M':
            data_load(set_index, line_index, tag, miss, tags, valid,
                      dirty, usage_queue, usage_table);
            data_store(set_index, line_index, tag, 0, tags, valid,
                       dirty, usage_queue, usage_table);
            break;
         }
      }

      verbose_print("\n");
   }


   printSummary(hits, misses, evictions, dirty_evicted,
                dirty_active, double_accesses);

   // freeing pointers
   free(tags);
   free(valid);
   free(dirty);
   free(usage_queue);
   for(i=0; i<(1 << set_bits)*assoc; ++i) {
      free(usage_table[i]);
   }
   free(usage_table);
   return 0;
}



void get_opt_args(int argc, char* argv[]) {
   // Reading function arguments
   int opt;
   while ((opt = getopt(argc, argv, "vs:b:E:t:")) != -1) {
      switch(opt) {
         case 'v':
         verbose = 1;
         break;

         case 's':
         set_bits = atoi(optarg);
         break;

         case 'b':
         block_bits = atoi(optarg);
         break;

         case 'E':
         assoc = atoi(optarg);
         break;

         case 't':
         if(!(trace_file = fopen(optarg, "r"))) {
            fprintf(stderr, "Could not open file %s\n", optarg);
            exit(1); // could not open file.
         }
         break;

         default:
         fprintf(stderr, "Usage: %s [-v] -s <s> -E <E> -b <b> -t <tracefile>\n",
                  argv[0]);
         exit(1);
      }
   }
   if (optind > argc) {
      fprintf(stderr, "Expected argument after options %i, %i\n", optind, argc);
      exit(1);
   }
   if(!trace_file) {
      fprintf(stderr, "No trace file provided\n");
      exit(1);
   }
}



void verbose_print(char* str) {
   if(verbose) {
      printf("%s", str);
   }
}



void data_load(int set_index, int line_index, long tag, int miss, long* tags,
            char* valid, char* dirty, Queue* usage_queue, Node** usage_table) {
   int line = set_index*assoc+line_index;

   // checking hits
   if(!miss){
      cache_hit(line_index, set_index, usage_queue);
      move_front(&usage_queue[set_index], usage_table[line]);
   } else {
      misses++;
      verbose_print("miss ");

      tags[line]=tag;
      if(!valid[line]) { // line is not valid (cold miss)
         valid[line] = 1;
         usage_table[line] = enqueue(&usage_queue[set_index], line_index);
      } else { // line is valid but tag isn't (conflict miss)
         cache_eviction(line, dirty);
         move_front(&usage_queue[set_index], (&usage_queue[set_index])->tail);
      }
   }
}



void data_store(int set_index, int line_index, long tag, int miss, long* tags,
            char* valid,char* dirty, Queue* usage_queue, Node** usage_table) {
   int line = set_index*assoc+line_index; // index in the array

   // checking hits
   if(!miss){
      cache_hit(line_index, set_index, usage_queue);
      move_front(&usage_queue[set_index], usage_table[line]);
   } else {
      misses++;
      verbose_print("dirty miss ");

      tags[line]=tag;
      if(!valid[line]) { // line is not valid (cold miss)
         valid[line] = 1;
         usage_table[line] = enqueue(&usage_queue[set_index], line_index);
      } else { // line is valid but tag isn't (conflict miss)
         cache_eviction(line, dirty);
         move_front(&usage_queue[set_index], (&usage_queue[set_index])->tail);
      }
   }

   // data store will always set the dirty bit
   if(!dirty[line]) {
      dirty[line] = 1;
      dirty_active+=1 << block_bits;
   }
}



void cache_eviction(int line, char* dirty) {
   if(dirty[line]) {
      verbose_print("dirty-eviction ");
      dirty_active -= 1 << block_bits;
      dirty_evicted += 1 << block_bits;
      dirty[line] = 0;
   } else {
      verbose_print("eviction ");
   }

   evictions++;
}



void cache_hit(int line_index, int set_index, Queue* usage_queue) {
   hits++;
   if(line_index == peek_head(&usage_queue[set_index])) {
      double_accesses++;
      verbose_print("hit-double_ref ");
   } else {
      verbose_print("hit ");
   }
}



void initialize_queue(Queue* q, int max_size) {
   q->head = NULL;
   q->tail = NULL;
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


void move_front(Queue* q, Node* n) {
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
