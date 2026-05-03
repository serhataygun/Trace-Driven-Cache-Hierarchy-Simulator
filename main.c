#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>

void simulate(Cache *l1, Cache *l2, HierarchyMode hmode, TraceEntry *entries, long count, Stats *s1, Stats *s2)
{
    for (long i = 0; i < count; i++) {
        char op   = entries[i].op;
        long addr = entries[i].addr;

        int  n_accesses = (op == 'M') ? 2 : 1;
        int  is_write   = (op == 'S') ? 1 : 0;

        for (int a = 0; a < n_accesses; a++) {
            int this_write = (op == 'M') ? (a == 1 ? 1 : 0) : is_write;

            int l1_hit = cache_access(l1, addr, this_write, s1);

            if (!l1_hit && l2 != NULL) {
                int l2_hit = cache_access(l2, addr, this_write, s2);

                if (hmode == HIER_EXCLUSIVE && l2_hit) {
                    cache_invalidate(l2, addr);
                }
            }
        }
    }
}
