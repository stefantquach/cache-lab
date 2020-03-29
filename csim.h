#ifndef CSIM_H
#define CSIM_H
#include "queue.h"
/**************** Helper Functions ********************************/
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

#endif /* end of include guard: CSIM_H */
