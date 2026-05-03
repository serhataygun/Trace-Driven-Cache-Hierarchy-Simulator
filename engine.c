#include "cache.h"
#include <stdlib.h>

Cache *cache_init(int s, int E, int b, ReplacementPolicy rpol, WritePolicy wpol, const char *name)
{
    Cache *c     = malloc(sizeof(Cache));
    c->s = s; c->E = E; c->b = b;
    c->S = 1 << s;
    c->B = 1 << b;
    c->rpol = rpol;
    c->wpol = wpol;
    c->name = name;
    c->sets  = calloc(c->S, sizeof(CacheSet));
    for (int i = 0; i < c->S; i++) {
        c->sets[i].lines = calloc(E, sizeof(CacheLine));
        c->sets[i].fifo_counter = 0;
        for (int j = 0; j < E; j++) {
            c->sets[i].lines[j].valid = 0;
            c->sets[i].lines[j].dirty = 0;
            c->sets[i].lines[j].tag   = -1;
            c->sets[i].lines[j].order = 0;
        }
    }
    return c;
}

void cache_free(Cache *c)
{
    for (int i = 0; i < c->S; i++) free(c->sets[i].lines);
    free(c->sets);
    free(c);
}

static int pick_victim(CacheSet *set, int E, ReplacementPolicy rpol)
{
    if (rpol == POL_RANDOM) {
        return rand() % E;
    }
    int victim = 0, min_order = set->lines[0].order;
    for (int i = 1; i < E; i++) {
        if (set->lines[i].order < min_order) {
            min_order = set->lines[i].order;
            victim = i;
        }
    }
    return victim;
}

int cache_access(Cache *c, long addr, int is_write, Stats *st)
{
    long set_index = (addr >> c->b) & (c->S - 1);
    long tag       = addr >> (c->b + c->s);
    CacheSet *set  = &c->sets[set_index];

    st->total++;
    if (is_write) st->write_accesses++;
    else          st->read_accesses++;

    /* ── HIT CHECK ── */
    for (int i = 0; i < c->E; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            st->hits++;
            if (is_write) {
                if (c->wpol == WP_WRITEBACK)
                    set->lines[i].dirty = 1;
            }
            if (c->rpol == POL_LRU)
                set->lines[i].order = set->fifo_counter++;
            return 1; /* HIT */
        }
    }

    /* ── MISS ── */
    st->misses++;

    if (is_write && c->wpol == WP_WRITETHROUGH) {
        return 0; /* miss, no allocation */
    }

    /* Find an empty (invalid) line first */
    for (int i = 0; i < c->E; i++) {
        if (!set->lines[i].valid) {
            set->lines[i].valid = 1;
            set->lines[i].dirty = is_write ? (c->wpol == WP_WRITEBACK ? 1 : 0) : 0;
            set->lines[i].tag   = tag;
            set->lines[i].order = set->fifo_counter++;
            return 0; /* miss, cold fill */
        }
    }

    /* ── EVICTION ── */
    st->evictions++;
    int victim = pick_victim(set, c->E, c->rpol);

    if (set->lines[victim].dirty) {
        st->dirty_evictions++;
    }

    set->lines[victim].valid = 1;
    set->lines[victim].dirty = is_write ? (c->wpol == WP_WRITEBACK ? 1 : 0) : 0;
    set->lines[victim].tag   = tag;
    set->lines[victim].order = set->fifo_counter++;

    return 0; /* miss + eviction */
}

void cache_invalidate(Cache *c, long addr)
{
    long set_index = (addr >> c->b) & (c->S - 1);
    long tag       = addr >> (c->b + c->s);
    CacheSet *set  = &c->sets[set_index];

    for (int i = 0; i < c->E; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            set->lines[i].valid = 0;
            set->lines[i].dirty = 0;
            return;
        }
    }
}
