# cantorDB

## Introduction

CantorDB is a (for now) in-memory, set-theoretic database written in C++. Rathher that organizing data round tables, rows, or graph edges, it takes its conceptual ground from naiv set theory.
The core primitiveis the set which is a named node that can contain other sets within it, belong to other sets and have key value properties. Every set created is automatically part of the universal set on creation, giving you a global namespace for enumeration and complement operations.
The database is named in honor of George Cantor, whose insight was that sets was not just a part of the mathematical world but rather the fabric upon which it was based. CantorDB is making the same proposition as it relates to databases.

## Conceptual foundation

This database is unique in that it is based upon sets. While in a relational database, a record exists as a row in a table and its connections to other data is expressed through foreign keys keys and junction tables, which are necessary sacrifices to the model. In a graph database, nodes and edges are distinct primitives; a relationship is ancilary to the node rather than intrinsic within the node. However CantorDB takes a different conceptual framework. Instead, the set is the only primitive data element, and membership is the data structure. Theoretically, the key value pairs are unnesessary as one could express them through microsets, however, this could lead to bloat and as such key value pairs were included for data which is likely unique.
Every entity within CantorDB is a set. Every set can belong to other sets and contain other sets as elements simultaneously. There is no seam or difference between a node existing and having a relationship between sets. To create a set is to automatically create a membership between it and the universal set and place it within a web of potential memberhips. This means complicated many-to-many relationships are a natural consequence of the model rather than requiring an engineering workaround.
This also makes the query model different in quality, and not just in syntax. It is based purely upon set algebra as the primary retrieval model. You do not have to write a join, you can instead describe a union or other relationship between the sets and allow the algebera to return it.
The universal set makes this possible in a principled way. CantorDB maintains a closed-world assumption, that is, every set within the database, barring ones generated from set operations, is contained within the universal set. THis is what gives complement and difference their conceptual meaning. A complement is not just everything which wasn't selected, rather it is the precise mathematical remainder against a known totality.

## Building

This can be added to any project via including the files within your project. It uses C++ 17 but is otherwise completely dependency free. As it is not header only, it does require linking. I have compiled it both on Windows 10 and Omarchy.

## Basic Usage

```cpp
#include "cantordb.h"

cantordb db("my_database");

// Create sets
db.create_set("animals");
db.create_set("dogs");
db.create_set("cats");
db.create_set("rex");
db.create_set("whiskers");

// Build membership hierarchy
db.add_member("animals", "dogs");
db.add_member("animals", "cats");
db.add_member("dogs", "rex");
db.add_member("cats", "whiskers");

// Attach properties
db.add_property("name", string("Rex"), "rex");
db.add_property("age", 4, "rex");
db.add_property("weight", 28.5, "rex");
db.add_property("vaccinated", true, "rex");

// Read properties
string name = db.get_property_safe_string("rex", "name");
int age     = db.get_property_safe_int("rex", "age");

// Set algebra
Set* result = db.set_union("dogs", "cats");      // all dogs and cats
Set* common = db.set_intersection("dogs", "cats"); // members of both
bool sub    = db.is_subset("dogs", "animals");   // true

db.clear_cache(); // free results when done

// Persist
save_cantordb(db, "zoo.cdb");
```
---

## API Reference

### Construction

```cpp
cantordb(string name)
```
Creates a new database. Initializes the `__universal__` and `__cache__` internal sets automatically.

---

### Set Management

```cpp
bool create_set(string set_name)
```
Creates a named set and registers it in the universal set. Returns `false` if the name already exists (`ER_SET_EX`).

```cpp
bool create_set(string set_name, bool gen_set)
```
Creates a set. If `gen_set` is `true`, the set is internal and is **not** added to the universal set. Used for system sets.

```cpp
bool delete_set(string set_name)
```
Removes the set and unlinks it from all parent and child sets. Returns `false` if not found (`ER_SET_NOT_FOUND`).

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
int get_cardinality(string set_name)
```
Returns the number of direct members. Returns `-1` if the set is not found.

---

### Membership

```cpp
bool add_member(string set_name, string element_name)
```
Makes `element_name` a member of `set_name`. Both sets must already exist. Updates both `has_element` and `member_of` vectors bidirectionally.

```cpp
bool remove_member(string set_name, string element_name)
```
Removes the membership relationship. Returns `false` if either set is not found or the element is not a member (`ER_ELEM_NOT_MEM`).

---

### Set Algebra

All algebra operations return a `Set*` that is owned by the cache. Call `clear_cache()` to free them. Returns `nullptr` on error.

```cpp
Set* set_union(string set_a_name, string set_b_name)
```
Returns a new set containing all elements from both sets (no duplicates). Named `"A ∪ B"`.

```cpp
Set* set_intersection(string set_a_name, string set_b_name)
```
Returns a new set containing only elements present in both. Named `"A ∩ B"`.

```cpp
Set* set_difference(string set_a_name, string set_b_name)
```
Returns elements in A that are not in B. Named `"A - B"`.

```cpp
Set* symmetric_difference(string set_a_name, string set_b_name)
```
Returns elements in either set but not both. Named `"A ⊕ B"`.

```cpp
Set* set_complement(string set_name)
```
Returns all sets in the universal set that are not members of the given set. Named `"A'"`.

```cpp
bool is_subset(string set_a_name, string set_b_name)
```
Returns `true` if every element of A is also in B.

```cpp
bool is_superset(string set_a_name, string set_b_name)
```
Returns `true` if every element of B is also in A.

```cpp
bool is_proper_subset(string set_a_name, string set_b_name)
```
Returns `true` if A is a subset of B and A ≠ B.

```cpp
bool is_equal(string set_a_name, string set_b_name)
```
Returns `true` if both sets have identical members.

```cpp
bool is_disjoint(string set_a_name, string set_b_name)
```
Returns `true` if the intersection of A and B is empty.

---

### Properties

Properties are typed key-value pairs attached to a set. Each key maps to exactly one type and cannot change type after creation.

**Supported types:** `int`, `long`, `double`, `string`, `bool`

```cpp
bool add_property(string key, T value, string set_name)
```
Adds a new property. Fails if the key already exists (`ER_KEY_EX`) or the set is not found.

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
string list_property_keys(string set_name)
```
Returns all keys as newline-delimited `key: type` entries.

```cpp
string list_string_properties(string set_name)
string list_int_properties(string set_name)
string list_double_properties(string set_name)
string list_bool_properties(string set_name)
string list_long_properties(string set_name)
```
Returns newline-delimited `key:value` entries filtered to that type.

---

### Cache

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

## Included Dataset

Included as an example data set. It can be loaded directly using the code snippet below.

```cpp
#include "cantordb.h"
#include <iostream>
using namespace std;

int main() {
    cantordb* db = load_cantordb("periodic_table.cdb");
    if (!db) {
        cout << "Failed to load database." << endl;
        return 1;
    }

    // List all ferromagnetic elements
    cout << db->list_elements("Ferromagnetic") << endl;

    // Show Iron's properties
    cout << db->list_properties("Iron") << endl;

    delete db;
    return 0;
}
```

It includes all known elements along with sets describing many of their properties.

### Dataset Contents

**Category Hierarchy**

Elements are organized into a two-level type hierarchy:

- `Metals` — contains `AlkaliMetals`, `AlkalineEarthMetals`, `TransitionMetals`, `PostTransitionMetals`, `Lanthanides`, `Actinides`
- `Nonmetals` — contains `ReactiveNonmetals`, `NobleGases`
- `Metalloids` — no parent grouping

Each element belongs to both its subcategory and the parent (`Metals` or `Nonmetals`).

**Cross-Cutting Sets**

| Category | Sets |
|---|---|
| State at room temperature | `Solid`, `Liquid`, `Gas` |
| Stability | `Radioactive`, `Stable` |
| Origin | `Natural`, `Synthetic` |
| Magnetic | `Ferromagnetic`, `Paramagnetic`, `Diamagnetic` |
| Molecular | `Diatomic` |
| Periods | `Period1` through `Period7` |
| Groups | `Group1` through `Group18` |

Note: Lanthanides and Actinides carry `group: 0` as a property and are not members of any Group set.

**Element Properties**

Every element carries these five properties:

| Key | Type |
|---|---|
| `atomic_number` | int |
| `atomic_weight` | double |
| `symbol` | string |
| `period` | int |
| `group` | int |

**Category Properties**

Category sets carry a `description` string and, where applicable, a `typical_charge` int.

**Example Queries**
```cpp
// All radioactive transition metals
Set* result = db->set_intersection("Radioactive", "TransitionMetals");

// All metals that are not transition metals
Set* result = db->set_difference("Metals", "TransitionMetals");

// All Period 4 elements that are ferromagnetic
Set* a      = db->set_intersection("Period4", "Ferromagnetic");
```

## AI policy

This database is a hobby project of mine due to my love for set theory. As such, I use AI to automate parts that I do not like such as writing tests or pushing commits. As such, while Claude is a co-author, and did assist in catching my silly mistakes, I handwrote this code and have no plans of using AI as anything but a glorified find and replace or writing tests. I certainly am not vibecoding it.

## Roadmap

My primary goal at this point is to design and impliment a SeQL (Set Query Language) for ease of using within a commandline environment. In addition, I would like to create R and Python bindings to enable its use for data science. I do have a few pie in the sky ideas such as allowing calculated sets to fully encapsulate the full set theory model which I may impliment later.

## Licence

This project uses an MIT licence.
