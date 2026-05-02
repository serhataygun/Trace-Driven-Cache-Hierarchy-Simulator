#include <stdio.h>
#include <stdlib.h>

#define COL_RESET  "\033[0m"
#define COL_RED    "\033[31m"

typedef struct {
    char op;
    long addr;
    int  size;
} TraceEntry;

TraceEntry *parse_trace_file(const char *path, long *count)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, COL_RED "Error: cannot open trace file '%s'\n" COL_RESET, path);
        exit(1);
    }

    long n = 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == ' ' || line[0] == 'L' ||
            line[0] == 'S' || line[0] == 'M') n++;
    }
    rewind(fp);

    TraceEntry *entries = malloc(n * sizeof(TraceEntry));
    if (!entries) { fprintf(stderr, "malloc failed\n"); exit(1); }

    long idx = 0;
    while (fgets(line, sizeof(line), fp) && idx < n) {
        char op;
        long addr;
        int  sz;
        if (sscanf(line, " %c %lx,%d", &op, &addr, &sz) != 3) continue;
        if (op == 'I') continue;
        entries[idx].op   = op;
        entries[idx].addr = addr;
        entries[idx].size = sz;
        idx++;
    }
    fclose(fp);

    *count = idx;
    return entries;
}
