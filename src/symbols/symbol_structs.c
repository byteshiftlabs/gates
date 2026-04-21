#include <string.h>
#include "symbol_structs.h"
#include "error_handler.h"
#include "utils.h"

// -------------------------------------------------------------
// Global State
// -------------------------------------------------------------
// Note: Global struct table for single-threaded, single-file compilation.
// Provides O(1) lookup during code generation for struct type info.
// For multi-threaded usage, refactor to use a CompilerContext struct.
// Call reset_struct_table() between independent compilations.
// -------------------------------------------------------------
static StructInfo g_structs[MAX_STRUCTS];
static int g_struct_count = 0;

int register_struct(const char *name)
{
    if (!name) {
        log_error(ERROR_CATEGORY_SEMANTIC, 0, "Cannot register struct with NULL name");
        return -1;
    }
    if (g_struct_count >= MAX_STRUCTS) {
        log_error(ERROR_CATEGORY_SEMANTIC, 0,
                  "Struct table full (max %d), cannot register '%s'",
                  MAX_STRUCTS, name);
        return -1;
    }

    // Check for duplicate
    int existing = find_struct_index(name);
    if (existing >= 0) {
        return existing;
    }

    int index = g_struct_count;
    safe_copy(g_structs[index].name, sizeof(g_structs[index].name),
              name, sizeof(g_structs[index].name));
    g_structs[index].field_count = 0;
    g_struct_count++;
    return index;
}

int register_struct_field(int struct_index, const char *field_type, const char *field_name)
{
    if (struct_index < 0 || struct_index >= g_struct_count) {
        log_error(ERROR_CATEGORY_SEMANTIC, 0, "Invalid struct index %d for field registration", struct_index);
        return -1;
    }
    if (!field_type || !field_name) {
        log_error(ERROR_CATEGORY_SEMANTIC, 0, "Cannot register struct field with NULL type or name");
        return -1;
    }

    StructInfo *info = &g_structs[struct_index];
    if (info->field_count >= MAX_STRUCT_FIELDS) {
        log_error(ERROR_CATEGORY_SEMANTIC, 0,
                  "Struct '%s' fields full (max %d), cannot add '%s'",
                  info->name, MAX_STRUCT_FIELDS, field_name);
        return -1;
    }

    int field_index = info->field_count;
    safe_copy(info->fields[field_index].field_name,
              sizeof(info->fields[field_index].field_name),
              field_name, sizeof(info->fields[field_index].field_name));
    safe_copy(info->fields[field_index].field_type,
              sizeof(info->fields[field_index].field_type),
              field_type, sizeof(info->fields[field_index].field_type));
    info->field_count++;
    return 0;
}

int find_struct_index(const char *name)
{
    if (!name) {
        return -1;
    }

    for (int struct_idx = 0; struct_idx < g_struct_count; struct_idx++) {
        if (strcmp(g_structs[struct_idx].name, name) == 0) {
            return struct_idx;
        }
    }

    return -1;
}

const StructInfo* get_struct_info(int struct_index)
{
    if (struct_index < 0 || struct_index >= g_struct_count) {
        return NULL;
    }
    return &g_structs[struct_index];
}

int get_struct_count(void)
{
    return g_struct_count;
}

void reset_struct_table(void)
{
    memset(g_structs, 0, sizeof(g_structs));
    g_struct_count = 0;
}
