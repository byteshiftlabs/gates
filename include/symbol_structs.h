#ifndef SYMBOL_STRUCTS_H
#define SYMBOL_STRUCTS_H

/**
 * @file symbol_structs.h
 * @brief Struct symbol table for tracking declared structs and their fields.
 */

// Table and buffer limits
#define MAX_STRUCTS          64
#define MAX_STRUCT_FIELDS    32
#define STRUCT_NAME_LENGTH   64
#define FIELD_NAME_LENGTH    64
#define FIELD_TYPE_LENGTH    32

#ifdef __cplusplus
extern "C" {
#endif

// Struct metadata description
typedef struct {
    char name[STRUCT_NAME_LENGTH];
    struct {
        char field_name[FIELD_NAME_LENGTH];
        char field_type[FIELD_TYPE_LENGTH];
    } fields[MAX_STRUCT_FIELDS];
    int field_count;
} StructInfo;

/**
 * @brief Register a new struct in the global table.
 * @param name  Struct type name.
 * @return Index of the new struct, or -1 if the table is full.
 */
int register_struct(const char *name);

/**
 * @brief Register a field in an existing struct entry.
 * @param struct_index  Index returned by register_struct().
 * @param field_type    C type name of the field.
 * @param field_name    Field identifier.
 * @return 0 on success, -1 on failure (bad index or field table full).
 */
int register_struct_field(int struct_index, const char *field_type, const char *field_name);

/**
 * @brief Find the index of a struct by name in the global table.
 * @param name  Struct type name.
 * @return Index into g_structs, or -1 if not found.
 */
int find_struct_index(const char *name);

/**
 * @brief Get a read-only pointer to a struct's metadata.
 * @param struct_index  Index returned by register_struct() or find_struct_index().
 * @return Pointer to StructInfo, or NULL if index is out of range.
 */
const StructInfo* get_struct_info(int struct_index);

/**
 * @brief Get the current number of registered structs.
 * @return Count of structs in the table.
 */
int get_struct_count(void);

/**
 * @brief Reset the struct table (clear all entries).
 */
void reset_struct_table(void);

#ifdef __cplusplus
}
#endif

#endif // SYMBOL_STRUCTS_H
