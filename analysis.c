#include "cache.h"
#include <stdio.h>

static const char *rpol_name(ReplacementPolicy p) {
    switch (p) {
        case POL_LRU:    return "LRU";
        case POL_FIFO:   return "FIFO";
        case POL_RANDOM: return "RANDOM";
        default:         return "?";
    }
}

static const char *wpol_name(WritePolicy p) {
    switch (p) {
        case WP_WRITEBACK:    return "Write-Back / Write-Allocate";
        case WP_WRITETHROUGH: return "Write-Through / No-Write-Allocate";
        default:              return "?";
    }
}

static const char *hier_name(HierarchyMode m) {
    switch (m) {
        case HIER_L1_ONLY:   return "Single-Level (L1 only)";
        case HIER_INCLUSIVE: return "Two-Level Inclusive (L1 + L2)";
        case HIER_EXCLUSIVE: return "Two-Level Exclusive (L1 + L2)";
        default:             return "?";
    }
}

static void print_cache_config(Cache *c)
{
    int cap_kb = (c->S * c->E * c->B) / 1024;
    int cap_b  = (c->S * c->E * c->B) % 1024;
    printf("  " COL_CYAN "%-4s" COL_RESET
           "  Sets=%-4d  Ways=%-3d  BlockSize=%-4d bytes"
           "  Capacity=%d",
           c->name, c->S, c->E, c->B, cap_kb);
    if (cap_kb == 0) printf("%d B\n", cap_b);
    else             printf(" KB\n");
}

static void print_stats_block(const char *label, const Stats *st, long total_insns)
{
    double hit_rate  = st->total ? 100.0 * st->hits   / st->total : 0.0;
    double miss_rate = st->total ? 100.0 * st->misses / st->total : 0.0;

    printf("\n" COL_BOLD COL_BLUE "  ── %s ──" COL_RESET "\n", label);
    printf("    Total accesses    : %ld\n",  st->total);
    printf("    Reads             : %ld\n",  st->read_accesses);
    printf("    Writes            : %ld\n",  st->write_accesses);
    printf("    Hits              : " COL_GREEN "%ld" COL_RESET "\n", st->hits);
    printf("    Misses            : " COL_RED   "%ld" COL_RESET "\n", st->misses);
    printf("    Evictions         : %ld\n",  st->evictions);
    printf("    Dirty evictions   : " COL_YELLOW "%ld" COL_RESET "\n", st->dirty_evictions);
    printf("    Hit rate          : " COL_GREEN "%.2f%%" COL_RESET "\n",  hit_rate);
    printf("    Miss rate         : " COL_RED   "%.2f%%" COL_RESET "\n", miss_rate);

    (void)total_insns; 
}

static void print_cpi_estimate(const Stats *s1, const Stats *s2, long total_accesses)
{
    if (total_accesses == 0) return;

    long l1_misses      = s1->misses;
    long l2_hits        = s2 ? s2->hits   : 0;
    long mem_accesses   = s2 ? s2->misses : l1_misses;

    double stall = (double)l2_hits   * L2_HIT_CYCLES
                 + (double)mem_accesses * MEM_PENALTY_CYCLES;

    double cpi_est = BASE_CPI + stall / (double)total_accesses;

    printf("\n" COL_BOLD COL_MAGENTA "  ── CPI Estimate ──" COL_RESET "\n");
    printf("    Base CPI          : %.2f\n",  BASE_CPI);
    printf("    L2 hit penalty    : %d cycles/miss\n",  L2_HIT_CYCLES);
    printf("    Mem miss penalty  : %d cycles/miss\n",  MEM_PENALTY_CYCLES);
    printf("    L1 misses         : %ld\n",   l1_misses);
    printf("    L2 hits           : %ld\n",   l2_hits);
    printf("    Memory accesses   : %ld\n",   mem_accesses);
    printf("    Estimated CPI     : " COL_YELLOW "%.4f" COL_RESET "\n", cpi_est);
}

void print_summary(Cache *l1, Cache *l2, HierarchyMode hmode, ReplacementPolicy rpol, 
                   WritePolicy wpol, const char *tracefile, const Stats *s1, 
                   const Stats *s2, long total_entries)
{
    printf("\n");
    printf(COL_BOLD COL_CYAN
           "╔══════════════════════════════════════════════════════╗\n"
           "║     Trace-Driven Cache Hierarchy Simulator           ║\n"
           "║     Computer Architecture – Spring 2026              ║\n"
           "╚══════════════════════════════════════════════════════╝\n"
           COL_RESET);

    printf("\n" COL_BOLD "  Configuration\n" COL_RESET);
    printf("  Trace file         : %s\n",          tracefile);
    printf("  Total trace ops    : %ld\n",         total_entries);
    printf("  Hierarchy mode     : %s\n",          hier_name(hmode));
    printf("  Replacement policy : %s\n",          rpol_name(rpol));
    printf("  Write policy       : %s\n",          wpol_name(wpol));
    printf("\n" COL_BOLD "  Cache Geometry\n" COL_RESET);
    print_cache_config(l1);
    if (l2) print_cache_config(l2);

    printf("\n" COL_BOLD "  Simulation Results\n" COL_RESET);
    printf(COL_CYAN "  ──────────────────────────────────────────────────────\n" COL_RESET);

    print_stats_block("L1 Cache", s1, total_entries);
    if (l2 && s2) {
        print_stats_block("L2 Cache", s2, total_entries);
        printf("\n" COL_BOLD "  Combined L1+L2\n" COL_RESET);
        long total_hits = s1->hits + s2->hits;
        long total_miss = s2->misses;
        long total_acc  = s1->total;
        if (total_acc > 0) {
            printf("    Overall hit rate  : " COL_GREEN "%.2f%%" COL_RESET
                   "  (L1 hits + L2 hits out of all accesses)\n",
                   100.0 * total_hits / total_acc);
            printf("    Memory miss rate  : " COL_RED "%.2f%%" COL_RESET
                   "  (go-to-DRAM rate)\n",
                   100.0 * total_miss / total_acc);
        }
    }
    print_cpi_estimate(s1, s2, total_entries);

}
