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

static void usage(const char *prog)
{
    fprintf(stderr,
        COL_BOLD "Usage:" COL_RESET " %s [L1 options] [L2 options] [policy options] -t <tracefile>\n"
        "\n"
        COL_BOLD "L1 Cache (required):\n" COL_RESET
        "  -s <bits>      Set index bits  (sets = 2^s)\n"
        "  -E <ways>      Associativity   (1 = direct-mapped)\n"
        "  -b <bits>      Block offset bits (block size = 2^b bytes)\n"
        "\n"
        COL_BOLD "L2 Cache (optional, add --l2 to enable):\n" COL_RESET
        "  --l2           Enable two-level hierarchy\n"
        "  --l2-s <bits>  L2 set index bits  (default: s+2)\n"
        "  --l2-E <ways>  L2 associativity   (default: E*2)\n"
        "  --l2-b <bits>  L2 block bits      (default: b)\n"
        "  --exclusive    Use exclusive hierarchy (default: inclusive)\n"
        "\n"
        COL_BOLD "Policy options:\n" COL_RESET
        "  --policy <p>   Replacement: LRU (default) | FIFO | RANDOM\n"
        "  --write  <p>   Write policy: WB (default) | WT\n"
        "                   WB = Write-Back/Write-Allocate\n"
        "                   WT = Write-Through/No-Write-Allocate\n"
        "\n"
        COL_BOLD "Trace:\n" COL_RESET
        "  -t <file>      Valgrind/lackey trace file\n"
        "\n"
        COL_BOLD "Examples:\n" COL_RESET
        "  %s -s 4 -E 1 -b 5 -t trace.txt\n"
        "  %s -s 4 -E 4 -b 5 --policy FIFO --write WT -t trace.txt\n"
        "  %s -s 4 -E 4 -b 5 --l2 --l2-s 6 --l2-E 8 --l2-b 6 -t trace.txt\n"
        "  %s -s 4 -E 4 -b 5 --l2 --exclusive --policy RANDOM -t trace.txt\n",
        prog, prog, prog, prog, prog);
    exit(1);
}
