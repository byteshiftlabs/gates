#include <string.h>
#include "symbol_structs.h"

// Definition of global struct table
StructInfo g_structs[64];
int g_struct_count = 0;

int find_struct_index(const char *name)
{
    int struct_idx = 0;

    if (!name) {
        return -1;
    }

    for (struct_idx = 0; struct_idx < g_struct_count; struct_idx++) {
        if (strcmp(g_structs[struct_idx].name, name) == 0) {
            return struct_idx;
        }
    }

    return -1;
}

const char* struct_field_type(const char *struct_name, const char *field_name)
{
    int idx = -1;
    int field_index = 0;

    if (!struct_name || !field_name) {
        return NULL;
    }

    idx = find_struct_index(struct_name);

    if (idx < 0) {
        return NULL;
    }

    for (field_index = 0; field_index < g_structs[idx].field_count; field_index++) {
        if (strcmp(g_structs[idx].fields[field_index].field_name, field_name) == 0) {
            return g_structs[idx].fields[field_index].field_type;
        }
    }

    return NULL;
}
