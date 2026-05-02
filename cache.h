#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>
#include <stdlib.h>

/* constants and enumerations */

#define L1_HIT_CYCLES       4
#define L2_HIT_CYCLES      12
#define MEM_PENALTY_CYCLES 200
#define BASE_CPI            1.0

#define COL_RESET  "\033[0m"
#define COL_BOLD   "\033[1m"
#define COL_CYAN   "\033[36m"
#define COL_GREEN  "\033[32m"
#define COL_YELLOW "\033[33m"
#define COL_RED    "\033[31m"
#define COL_BLUE   "\033[34m"
#define COL_MAGENTA "\033[35m"

typedef enum { POL_LRU = 0, POL_FIFO, POL_RANDOM } ReplacementPolicy;
typedef enum { WP_WRITEBACK = 0, WP_WRITETHROUGH } WritePolicy;
typedef enum { HIER_L1_ONLY = 0, HIER_INCLUSIVE, HIER_EXCLUSIVE } HierarchyMode;

/* data structers */

typedef struct {
    char op;
    long addr;
    int  size;
} TraceEntry;

/* function prototypes */

/* parser.c */
TraceEntry *parse_trace_file(const char *path, long *count);
