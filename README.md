# cantorDB

## Introduction

CantorDB is a (for now) in-memory, set-theoretic database written in C++. Rather that organizing data round tables, rows, or graph edges, it takes its conceptual ground from naive set theory.
The core primitive is the set which is a named node that can contain other sets within it, belong to other sets and have key value properties. Every set created is automatically part of the universal set on creation, giving you a global namespace for enumeration and complement operations.
The database is named in honor of George Cantor, whose insight was that sets was not just a part of the mathematical world but rather the fabric upon which it was based. CantorDB is making the same proposition as it relates to databases.

## Conceptual foundation

This database is unique in that it is based upon sets. While in a relational database, a record exists as a row in a table and its connections to other data is expressed through foreign keys keys and junction tables, which are necessary sacrifices to the model. In a graph database, nodes and edges are distinct primitives; a relationship is ancilary to the node rather than intrinsic within the node. However CantorDB takes a different conceptual framework. Instead, the set is the only primitive data element, and membership is the data structure. Theoretically, the key value pairs are unnesessary as one could express them through microsets, however, this could lead to bloat and as such key value pairs were included for data which is likely unique.
Every entity within CantorDB is a set. Every set can belong to other sets and contain other sets as elements simultaneously. There is no seam or difference between a node existing and having a relationship between sets. To create a set is to automatically create a membership between it and the universal set and place it within a web of potential memberhips. This means complicated many-to-many relationships are a natural consequence of the model rather than requiring an engineering workaround.
This also makes the query model different in quality, and not just in syntax. It is based purely upon set algebra as the primary retrieval model. You do not have to write a join, you can instead describe a union or other relationship between the sets and allow the algebera to return it.
The universal set makes this possible in a principled way. CantorDB maintains a closed-world assumption, that is, every set within the database, barring ones generated from set operations, is contained within the universal set. THis is what gives complement and difference their conceptual meaning. A complement is not just everything which wasn't selected, rather it is the precise mathematical remainder against a known totality.

## Building

This can be added to any project via including the files within your project. It uses C++20 but is otherwise completely dependency free. As it is not header only, it does require linking. I have compiled it both on Windows 10 and Omarchy.

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

This database is a hobby project of mine due to my love for set theory. As such, I use AI to automate parts that I do not like such as writing tests or pushing commits. As such, while Claude is a co-author, and did assist in catching my silly mistakes, I handwrote this code and use it for tests, generating documentations, find and repalce, and commit messages. I certainly am not vibecoding it.

## Roadmap

My primary goal at this point is to design and implement a SeQL (Set Query Language) for ease of using within a commandline environment. In addition, I would like to create R and Python bindings to enable its use for data science. I do have a few pie in the sky ideas such as allowing calculated sets, which are sets which contain a test such as num % 2 == 1 being the set of odd numbers, to fully encapsulate the full set theory model which I may implement later.

## Licence

This project is licensed under the MIT License, see the License file for details.
