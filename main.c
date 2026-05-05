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

int main(int argc, char *argv[])
{
    srand((unsigned)time(NULL));

    int s = -1, E = -1, b = -1;
    int use_l2 = 0, l2s = -1, l2E = -1, l2b = -1, exclusive = 0;
    ReplacementPolicy rpol = POL_LRU;
    WritePolicy       wpol = WP_WRITEBACK;
    char *tracefile = NULL;

    static struct option long_opts[] = {
        {"l2",        no_argument,       NULL, 200},
        {"l2-s",      required_argument, NULL, 201},
        {"l2-E",      required_argument, NULL, 202},
        {"l2-b",      required_argument, NULL, 203},
        {"exclusive", no_argument,       NULL, 204},
        {"policy",    required_argument, NULL, 210},
        {"write",     required_argument, NULL, 211},
        {"help",      no_argument,       NULL, 'h'},
        {0, 0, 0, 0}
    };

    int opt, lidx;
    while ((opt = getopt_long(argc, argv, "s:E:b:t:h", long_opts, &lidx)) != -1) {
        switch (opt) {
            case 's': s = atoi(optarg); break;
            case 'E': E = atoi(optarg); break;
            case 'b': b = atoi(optarg); break;
            case 't': tracefile = optarg; break;
            case 200: use_l2 = 1; break;
            case 201: l2s = atoi(optarg); break;
            case 202: l2E = atoi(optarg); break;
            case 203: l2b = atoi(optarg); break;
            case 204: exclusive = 1; break;
            case 210:
                if      (!strcasecmp(optarg, "FIFO"))   rpol = POL_FIFO;
                else if (!strcasecmp(optarg, "RANDOM")) rpol = POL_RANDOM;
                else                                    rpol = POL_LRU;
                break;
            case 211:
                if (!strcasecmp(optarg, "WT")) wpol = WP_WRITETHROUGH;
                else                           wpol = WP_WRITEBACK;
                break;
            case 'h': 
            default:  usage(argv[0]);
        }
    }

    if (s < 0 || E < 1 || b < 0 || !tracefile)
        usage(argv[0]);

    if (use_l2) {
        if (l2s < 0) l2s = s + 2;
        if (l2E < 0) l2E = E * 2 < 1 ? 2 : E * 2;
        if (l2b < 0) l2b = b;
    }

    if (use_l2) {
        long l1_cap = (1L << s) * E * (1L << b);
        long l2_cap = (1L << l2s) * l2E * (1L << l2b);
        if (l2_cap <= l1_cap) {
            fprintf(stderr,
                COL_YELLOW "Warning: L2 capacity (%ld B) ≤ L1 capacity (%ld B). "
                "Results may be unrealistic.\n" COL_RESET, l2_cap, l1_cap);
        }
    }

    long count = 0;
    TraceEntry *entries = parse_trace_file(tracefile, &count);
    printf(COL_CYAN "  Parsed %ld trace entries from '%s'\n" COL_RESET, count, tracefile);

    Cache *l1 = cache_init(s, E, b, rpol, wpol, "L1");
    Cache *l2 = use_l2 ? cache_init(l2s, l2E, l2b, rpol, wpol, "L2") : NULL;

    HierarchyMode hmode = use_l2 ? (exclusive ? HIER_EXCLUSIVE : HIER_INCLUSIVE) : HIER_L1_ONLY;

    Stats s1 = {0}, s2 = {0};
    simulate(l1, l2, hmode, entries, count, &s1, &s2);

    print_summary(l1, l2, hmode, rpol, wpol, tracefile, &s1, use_l2 ? &s2 : NULL, count);

    free(entries);
    cache_free(l1);
    if (l2) cache_free(l2);

    return 0;
}
