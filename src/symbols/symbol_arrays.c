#include <string.h>
#include "symbol_arrays.h"
#include "error_handler.h"

static ArrayInfo g_arrays[MAX_ARRAYS];
static int g_array_count = 0;

int find_array_size(const char *name)
{
    if (!name) {
        return -1;
    }

    for (int array_idx = 0; array_idx < g_array_count; array_idx++) {
        if (strcmp(g_arrays[array_idx].name, name) == 0) {
            return g_arrays[array_idx].size;
        }
    }

    return -1;
}

void register_array(const char *name, int size)
{
    if (!name || size <= 0) {
        log_error(ERROR_CATEGORY_SEMANTIC, 0,
                  "Invalid array registration: name=%s, size=%d",
                  name ? name : "(null)", size);
        return;
    }
    if (g_array_count >= (int)(sizeof(g_arrays) / sizeof(g_arrays[0]))) {
        log_error(ERROR_CATEGORY_SEMANTIC, 0,
                  "Array table full (max %d), cannot register '%s'",
                  (int)(sizeof(g_arrays) / sizeof(g_arrays[0])), name);
        return;
    }

    // Prevent duplicates; update size if already present
    for (int array_idx = 0; array_idx < g_array_count; array_idx++) {
        if (strcmp(g_arrays[array_idx].name, name) == 0) {
            g_arrays[array_idx].size = size;
            return;
        }
    }

    strncpy(g_arrays[g_array_count].name, name, sizeof(g_arrays[g_array_count].name) - 1);
    g_arrays[g_array_count].name[sizeof(g_arrays[g_array_count].name) - 1] = '\0';
    g_arrays[g_array_count].size = size;
    g_array_count++;
}

int get_array_count(void)
{
    return g_array_count;
}

void reset_array_table(void)
{
    memset(g_arrays, 0, sizeof(g_arrays));
    g_array_count = 0;
}
