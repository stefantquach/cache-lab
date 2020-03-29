#include "cachelab.h"
#include "csim.h"
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

// Parameters
int verbose = 0;
int set_bits = 0;
int associativity = 1;
int block_bits = 0;
FILE* trace_file;

// Results
int hits = 0;
int misses = 0;
int evictions = 0;
int dirty_evicted = 0;
int dirty_active = 0;
int double_accesses = 0;

int main(int argc, char* argv[])
{
    int i;
    get_opt_args(argc, argv);
    // printf("verbose: %i, set_bits: %i, block_bits: %i, associativity: %i, trace_file ptr: %p\n",
    //         verbose, set_bits, block_bits, associativity, trace_file);

    // masks to access certain parts of address
    unsigned long block_mask = (1 << block_bits)-1;
    unsigned long set_mask = ((1 << set_bits)-1) << block_bits;

    // allocating memory for cache
    long* tags = (long*)malloc((1 << set_bits)*associativity*sizeof(long)); // S*E lines
    char* valid = (char*)malloc((1 << set_bits)*associativity*sizeof(char));
    char* dirty = (char*)malloc((1 << set_bits)*associativity*sizeof(char));

    // for tracking usage
    Queue* usage_queue = (Queue*)malloc((1 << set_bits)*sizeof(Queue)); // a Queue for each set
    Node** usage_table = (Node**)malloc((1 << set_bits)*associativity*sizeof(Node*)); // Hash table for quick access

    // reading trace file
    char type;
    unsigned long tr_addr;
    int num_bytes;
    // int last_line=-1;
    while(fscanf(trace_file, " %c %lx,%i", &type, &tr_addr, &num_bytes) != -1) {
        int line_index;
        int cold_index;
        int miss;
        if(verbose) {
            printf("%c %lx,%i ", type, tr_addr, num_bytes);
        }
        unsigned int set_index = (tr_addr & set_mask) >> block_bits;
        // int block_index = (tr_addr & block_mask);
        unsigned long tag = (tr_addr & ~(set_mask | block_mask)) >> (set_bits+block_bits);

        if(type != 'I') {
            // finding if there is a cache hit or miss
            line_index = -1;
            cold_index = -1;
            for(i=0; i<associativity; ++i) {
                // cache hit (should only run once)
                if(tag == tags[set_index*associativity+i] && valid[set_index*associativity+i]) {
                    line_index=i;
                }

                // storing the first unfilled line
                if(!valid[set_index*associativity+i] && cold_index == -1) {
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
                data_load(set_index, line_index, tag, miss, tags, valid, dirty, usage_queue, usage_table);
                break;

                case 'S':
                data_store(set_index, line_index, tag, miss, tags, valid, dirty, usage_queue, usage_table);
                break;

                case 'M':
                data_load(set_index, line_index, tag, miss, tags, valid, dirty, usage_queue, usage_table);
                data_store(set_index, line_index, tag, 0, tags, valid, dirty, usage_queue, usage_table);
                break;
            }
        }

        verbose_print("\n");
    }


    printSummary(hits, misses, evictions, dirty_evicted, dirty_active, double_accesses);

    // freeing pointers
    free(tags);
    free(valid);
    free(dirty);
    free(usage_queue);
    for(i=0; i<(1 << set_bits)*associativity; ++i) {
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
            associativity = atoi(optarg);
            break;

            case 't':
            if(!(trace_file = fopen(optarg, "r"))) {
                fprintf(stderr, "Could not open file %s\n", optarg);
                exit(1); // could not open file.
            }
            break;

            default:
            fprintf(stderr, "Usage: %s [-v] -s <s> -E <E> -b <b> -t <tracefile>\n", argv[0]);
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
    int line = set_index*associativity+line_index;

    // checking hits
    if(!miss){
        cache_hit(line_index, set_index, usage_queue);
        move_to_front(&usage_queue[set_index], usage_table[set_index*associativity+line_index]);
    } else {
        misses++;
        verbose_print("miss ");

        tags[line]=tag;
        if(!valid[line]) { // line is not valid (cold miss)
            valid[line] = 1;
            usage_table[set_index*associativity+line_index] = enqueue(&usage_queue[set_index], line_index);
        } else { // line is valid but tag isn't (conflict miss)
            cache_eviction(line, dirty);
            move_to_front(&usage_queue[set_index], (&usage_queue[set_index])->tail);
        }
    }
}



void data_store(int set_index, int line_index, long tag, int miss, long* tags,
                char* valid, char* dirty, Queue* usage_queue, Node** usage_table) {
    int line = set_index*associativity+line_index; // index in the array

    // checking hits
    if(!miss){
        cache_hit(line_index, set_index, usage_queue);
        move_to_front(&usage_queue[set_index], usage_table[set_index*associativity+line_index]);
    } else {
        misses++;
        verbose_print("dirty miss ");

        tags[line]=tag;
        if(!valid[line]) { // line is not valid (cold miss)
            valid[line] = 1;
            usage_table[set_index*associativity+line_index] = enqueue(&usage_queue[set_index], line_index);
        } else { // line is valid but tag isn't (conflict miss)
            cache_eviction(line, dirty);
            move_to_front(&usage_queue[set_index], (&usage_queue[set_index])->tail);
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
