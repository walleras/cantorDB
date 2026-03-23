# CantorDB API Reference

## Construction

```cpp
cantordb(string name)
```
Creates a new database. Initializes the `__universal__`, `__cache__`, and `__trash__` internal sets automatically.

```cpp
cantordb(string name, size_t memory_limit_bytes)
```
Creates a new database with a memory limit. If the limit is exceeded, `emergency_shut_off` is set to `true` and all mutating operations will fail.

---

## Set Management

```cpp
bool create_set(string set_name)
```
Creates a named set and registers it in the universal set. Returns `false` if the name already exists (`ER_SET_EX`).

```cpp
bool create_set(string set_name, bool gen_set)
```
Creates a set. If `gen_set` is `true`, the set is internal and is **not** added to the universal set. Used for system sets.

```cpp
bool delete_set(string set_name, bool safe = true)
```
Removes a set. If `safe` is `true` (default), the set is soft-deleted to trash, preserving membership for possible restore. If `false`, the set is permanently destroyed. Returns `false` if not found (`ER_SET_NOT_FOUND`).

```cpp
bool restore_set(string set_name)
```
Restores a soft-deleted set from the trash, re-linking all its previous parent and child memberships. Returns `false` if the set is not in the trash.

```cpp
bool rename_set(string set_name, string set_new_name)
```
Renames a set in-place. Returns `false` if the original is not found or the new name is already taken.

```cpp
string list_all_sets()
```
Returns a newline-delimited string of all set names in the universal set.

```cpp
string list_elements(string set_name)
```
Returns a newline-delimited string of direct member names of the given set.

```cpp
string list_sets(string set_name)
```
Returns a newline-delimited string of all parent sets that contain the given set as a member.

```cpp
int get_cardinality(string set_name)
```
Returns the number of direct members. Returns `-1` if the set is not found.

---

## Membership

```cpp
bool add_member(string set_name, string element_name)
```
Makes `element_name` a member of `set_name`. Both sets must already exist. Updates both `has_element` and `member_of` vectors bidirectionally.

```cpp
void add_member(string set_name, Set* element)
```
Adds a `Set*` directly as a member. Used internally for cache and generated result sets.

```cpp
bool remove_member(string set_name, string element_name)
```
Removes the membership relationship. Returns `false` if either set is not found or the element is not a member (`ER_ELEM_NOT_MEM`).

```cpp
bool is_element(string set_a_name, string set_b_name)
```
Returns `true` if `set_a` is a direct member of `set_b`.

---

## Set Algebra

All algebra operations return a `Set*` that is owned by the cache. Call `clear_cache()` to free them. Returns `nullptr` on error. Each operation has both a string name overload and a `Set*` pointer overload.

```cpp
Set* set_union(string set_a_name, string set_b_name)
Set* set_union(Set* set_a, Set* set_b)
```
Returns a new set containing all elements from both sets (no duplicates). Named `"A ∪ B"`.

```cpp
Set* set_intersection(string set_a_name, string set_b_name)
Set* set_intersection(Set* set_a, Set* set_b)
```
Returns a new set containing only elements present in both. Named `"A ∩ B"`.

```cpp
Set* set_difference(string set_a_name, string set_b_name)
Set* set_difference(Set* set_a, Set* set_b)
```
Returns elements in A that are not in B. Named `"A - B"`.

```cpp
Set* symmetric_difference(string set_a_name, string set_b_name)
Set* symmetric_difference(Set* set_a, Set* set_b)
```
Returns elements in either set but not both. Named `"A ⊕ B"`.

```cpp
Set* set_complement(string set_name)
```
Returns all sets in the universal set that are not members of the given set. Named `"A'"`.

```cpp
bool is_subset(string set_a_name, string set_b_name)
bool is_subset(Set* set_a, Set* set_b)
```
Returns `true` if every element of A is also in B.

```cpp
bool is_superset(string set_a_name, string set_b_name)
bool is_superset(Set* set_a, Set* set_b)
```
Returns `true` if every element of B is also in A.

```cpp
bool is_proper_subset(string set_a_name, string set_b_name)
bool is_proper_subset(Set* set_a, Set* set_b)
```
Returns `true` if A is a subset of B and A ≠ B.

```cpp
bool is_equal(string set_a_name, string set_b_name)
bool is_equal(Set* set_a, Set* set_b)
```
Returns `true` if both sets have identical members.

```cpp
bool is_disjoint(string set_a_name, string set_b_name)
bool is_disjoint(Set* set_a, Set* set_b)
```
Returns `true` if the intersection of A and B is empty.

---

## Properties

Properties are typed key-value pairs attached to a set. Each key must be registered with a type before it can be added to any set.

**Supported types:** `int`, `long`, `double`, `string`, `bool`

```cpp
bool create_property(string property_name, string type)
```
Registers a property name with a type system-wide. Must be called before `add_property`. Returns `false` if already registered.

```cpp
bool add_property(string key, T value, string set_name)
```
Adds a new property with a value. Fails if the key already exists (`ER_KEY_EX`), the set is not found, or the property has not been registered.

```cpp
bool add_property(string property_name, string set_name)
```
Adds a registered property to a set with a zero-initialized default value (0, "", 0.0, false, or 0L depending on type).

```cpp
bool update_property(string set_name, string key_name, T value)
```
Updates an existing property. Fails on type mismatch (`ER_KEY_WRONG_TYPE`).

```cpp
bool delete_property(string set_name, string key_name)
```
Removes a property by key.

```cpp
variant get_property(string set_name, string key_name)
```
Returns the property value as a `variant`. Requires `std::get<T>()` or `std::visit` to unpack. Returns `false` on error.

```cpp
int    get_property_safe_int(string set_name, string key_name)
string get_property_safe_string(string set_name, string key_name)
double get_property_safe_double(string set_name, string key_name)
bool   get_property_safe_bool(string set_name, string key_name)
long   get_property_safe_long(string set_name, string key_name)
```
Type-safe accessors. Return a zero/empty default and set `EC` to `ER_KEY_WRONG_TYPE` if the key holds a different type.

```cpp
string list_properties(string set_name)
```
Returns all properties as newline-delimited `key:value:type` entries.

```cpp
string list_all_keys()
```
Returns all registered property names system-wide as newline-delimited `key: type` entries.

```cpp
string list_property_keys(string set_name)
```
Returns all keys on a specific set as newline-delimited `key: type` entries.

```cpp
string list_string_properties(string set_name)
string list_int_properties(string set_name)
string list_double_properties(string set_name)
string list_bool_properties(string set_name)
string list_long_properties(string set_name)
```
Returns newline-delimited `key:value` entries filtered to that type.

---

## Where Queries

Filter elements of a set by property value. All return a `Set*` owned by the cache, or `nullptr` on error.

```cpp
Set* where_elements_greater_than(string set_name, string property, int value)
Set* where_elements_greater_than(string set_name, string property, long value)
Set* where_elements_greater_than(string set_name, string property, double value)
```
Returns elements where the property is strictly greater than the given value.

```cpp
Set* where_elements_lesser_than(string set_name, string property, int value)
Set* where_elements_lesser_than(string set_name, string property, long value)
Set* where_elements_lesser_than(string set_name, string property, double value)
```
Returns elements where the property is strictly less than the given value.

```cpp
Set* where_elements_equal_than(string set_name, string property, int value)
Set* where_elements_equal_than(string set_name, string property, long value)
Set* where_elements_equal_than(string set_name, string property, double value)
Set* where_elements_equal_than(string set_name, string property, string value)
Set* where_elements_equal_than(string set_name, string property, bool value)
```
Returns elements where the property equals the given value. Supports all five property types.

---

## Cache

Set algebra results are stored in the internal `__cache__` set. They are not persisted by `save_cantordb`.

```cpp
string list_cached_sets()
```
Returns newline-delimited names of all cached result sets.

```cpp
bool clear_cache()
```
Deletes all cached sets and frees their memory.

---

## Error Handling

All mutating methods return `bool` (or `nullptr` for pointer-returning methods) on failure. After any failed call, check the public fields:

```cpp
db.EC;             // ERROR_CODE enum value
db.error_message;  // human-readable description
```

### Error Codes

| Code | Meaning |
|------|---------|
| `ER_SET_EX` | A set with that name already exists |
| `ER_SET_NOT_FOUND` | No set with that name exists |
| `ER_KEY_EX` | A property with that key already exists on the set |
| `ER_KEY_NOT_FOUND` | No property with that key exists on the set |
| `ER_KEY_WRONG_TYPE` | The key exists but holds a different type than requested |
| `ER_ELEM_NOT_MEM` | The element is not a member of the given set |

### Example

```cpp
if (!db.add_property("age", 5, "rex")) {
    if (db.EC == ER_KEY_EX) {
        db.update_property("rex", "age", 5);
    } else {
        cerr << db.error_message << endl;
    }
}
```

---

## Persistence

cantorDB uses a compact binary format. The cache is intentionally excluded from saves.

```cpp
bool save_cantordb(const cantordb& db, const string& path)
```
Serializes all sets, their properties, and membership relationships to a binary file at `path`. Writes atomically via a `.tmp` file. Returns `false` on I/O failure.

```cpp
cantordb* load_cantordb(const string& path)
```
Reconstructs a database from a binary file. Returns a heap-allocated `cantordb*` on success, `nullptr` on failure. The caller owns the pointer.

```cpp
// Save
if (!save_cantordb(db, "zoo.cdb")) {
    cerr << "Save failed" << endl;
}

// Load
cantordb* db = load_cantordb("zoo.cdb");
if (!db) {
    cerr << "Load failed" << endl;
}
// ...use db...
delete db;
```

> **Note:** `load_cantordb` returns a raw pointer. Wrap it in a `unique_ptr` or ensure manual deletion to avoid leaks.
