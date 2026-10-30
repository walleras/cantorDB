#ifndef CDB_C_H
#define CDB_C_H

// C ABI exported by the C3 implementation (src/cantordb/rewrite/capi.c3).
// Strings in: NUL-terminated. Strings out: heap copies, release with cdb_free_cstr.
// Property value tags: 0=int 1=string 2=double 3=bool 4=long.

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cdb_db cdb_db;     // CantorDB*
typedef struct cdb_set cdb_set;   // Set*

cdb_db* cdb_new(const char* name, size_t memory_limit);
void cdb_delete(cdb_db* db);
cdb_db* cdb_load(const char* path);
bool cdb_save(cdb_db* db, const char* path);
char* cdb_name(cdb_db* db);
void cdb_free_cstr(cdb_db* db, char* p);

void cdb_clear_error(cdb_db* db);
bool cdb_has_error(cdb_db* db);
char* cdb_error_message(cdb_db* db);
int cdb_error_code(cdb_db* db);
bool cdb_shut_off(cdb_db* db);

bool cdb_create_set(cdb_db* db, const char* name, bool gen_set);
bool cdb_delete_set(cdb_db* db, const char* name, bool safe);
bool cdb_restore_set(cdb_db* db, const char* name);
bool cdb_rename_set(cdb_db* db, const char* from, const char* to);
bool cdb_add_member(cdb_db* db, const char* set, const char* elem);
bool cdb_remove_member(cdb_db* db, const char* set, const char* elem);
bool cdb_clear_cache(cdb_db* db);
cdb_set* cdb_find_set(cdb_db* db, const char* name);
char* cdb_set_name(cdb_db* db, cdb_set* s);
int cdb_cardinality(cdb_db* db, const char* name);

// op: 0=union 1=intersection 2=difference 3=symmetric difference
cdb_set* cdb_combine_named(cdb_db* db, const char* a, const char* b, int op);
cdb_set* cdb_combine(cdb_db* db, cdb_set* a, cdb_set* b, int op);
cdb_set* cdb_complement(cdb_db* db, const char* name);
// kind: 0=is_element(a of b) 1=disjoint 2=subset 3=superset 4=equal 5=proper subset
bool cdb_relation(cdb_db* db, const char* a, const char* b, int kind);

char* cdb_list_elements(cdb_db* db, const char* set);
char* cdb_list_sets(cdb_db* db, const char* set);
char* cdb_list_all_sets(cdb_db* db);
char* cdb_list_all_keys(cdb_db* db);
char* cdb_list_property_keys(cdb_db* db, const char* set);
char* cdb_list_properties(cdb_db* db, const char* set);

bool cdb_create_property(cdb_db* db, const char* name, const char* type);
bool cdb_delete_property(cdb_db* db, const char* set, const char* key);
bool cdb_add_property_default(cdb_db* db, const char* name, const char* set);
bool cdb_add_property(cdb_db* db, const char* key, int type, int64_t l, double d, const char* s, const char* set);
bool cdb_update_property(cdb_db* db, const char* set, const char* key, int type, int64_t l, double d, const char* s);
void cdb_get_property(cdb_db* db, const char* set, const char* key, int* type, int64_t* l, double* d, char** s);
int cdb_property_type(cdb_db* db, const char* name);
int cdb_key_type(cdb_db* db, const char* set, const char* key);
// op: 0=greater 1=lesser 2=equal
cdb_set* cdb_where(cdb_db* db, const char* set, const char* property, int op, int type, int64_t l, double d, const char* s);

#ifdef __cplusplus
}
#endif

#endif
