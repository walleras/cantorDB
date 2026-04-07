# cantorDB

## Introduction

CantorDB is an in-memory, set-theoretic database written in C++. Rather than organizing data around tables, rows, or graph edges, it takes its conceptual ground from naive set theory.
The core primitive is the set which is a named node that can contain other sets within it, belong to other sets and have key value properties. Every set created is automatically part of the universal set on creation, giving you a global namespace for enumeration and complement operations.
The database is named in honor of George Cantor, whose insight was that sets was not just a part of the mathematical world but rather the fabric upon which it was based. CantorDB is making the same proposition as it relates to databases.

## Conceptual foundation

This database is unique in that it is based upon sets. While in a relational database, a record exists as a row in a table and its connections to other data is expressed through foreign keys and junction tables, which are necessary sacrifices to the model. In a graph database, nodes and edges are distinct primitives; a relationship is ancillary to the node rather than intrinsic within the node. However CantorDB takes a different conceptual framework. Instead, the set is the only primitive data element, and membership is the data structure. Theoretically, the key value pairs are unnecessary as one could express them through microsets, however, this could lead to bloat and as such key value pairs were included for data which is likely unique.
Every entity within CantorDB is a set. Every set can belong to other sets and contain other sets as elements simultaneously. There is no seam or difference between a node existing and having a relationship between sets. To create a set is to automatically create a membership between it and the universal set and place it within a web of potential memberships. This means complicated many-to-many relationships are a natural consequence of the model rather than requiring an engineering workaround.
This also makes the query model different in quality, and not just in syntax. It is based purely upon set algebra as the primary retrieval model. You do not have to write a join, you can instead describe a union or other relationship between the sets and allow the algebra to return it.
The universal set makes this possible in a principled way. CantorDB maintains a closed-world assumption, that is, every set within the database, barring ones generated from set operations, is contained within the universal set. This is what gives complement and difference their conceptual meaning. A complement is not just everything which wasn't selected, rather it is the precise mathematical remainder against a known totality.

## Building

The library uses C++20 but is otherwise completely dependency free. It is not header-only and does require linking. I have compiled it both on Windows 10 and Omarchy.

To embed cantorDB in your own project, copy `src/cantordb/cantordb.h`, `cantordb.cpp`, `lexer.h`, `lexer.cpp`, `parser.h`, and `parser.cpp` into your project and compile them together.

A `CMakeLists.txt` is included. The default build produces the interactive shell (`cantordb`) and the test suite:

```sh
cmake -B build && cmake --build build
./build/cantordb         # interactive shell
./build/test_cantordb    # run tests
```

## Shell

CantorDB ships with an interactive REPL shell. Shell-level commands (usable before loading a database):

| Command | Description |
|---|---|
| `CREATE DATABASE <name>` | Create a new in-memory database |
| `LOAD <filename>` | Load a `.cdb` file (`.cdb` extension optional) |
| `UNLOAD` | Close the current database |
| `HELP` | Print a command reference |
| `QUIT` | Exit the shell |

Once a database is loaded, all SeQL queries are accepted at the prompt. Set names are **case-sensitive**: `Animals` and `animals` are different sets.

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

See [APIReference.md](APIReference.md) for the full C++ API documentation covering set management, membership, set algebra, properties, cache, error handling, and persistence.

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

This database is a hobby project of mine due to my love for set theory. As such, I use AI to automate parts that I do not like such as writing tests or pushing commits. While Claude is a co-author and did assist in catching my silly mistakes, I handwrote this code and use it for tests, generating documentation, find and replace, and commit messages. I certainly am not vibecoding it.

## Roadmap

SeQL and the interactive shell are implemented and working. The remaining item on the wishlist is Python bindings for data science use.

## Licence

This project is licensed under the MIT License, see the License file for details.
