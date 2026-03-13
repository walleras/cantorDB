# cantorDB TODO

## Essential (needed before query language)
- [x] `get_property` — read properties back through the API
- [x] `delete_property` — remove a key from a set
- [x] `update_property` — modify an existing key's value
- [x] `list_members` — enumerate a set's elements
- [x] `get_cardinality` — return element count of a set
- [x] `list_sets` — enumerate all sets in the db
- [x] `rename_set`
- [ ] `primitive elements` - Elements that don't have properties or members

## Nice to have (depends on query language scope)
- [ ] `cartesian_product` — A × B
- [ ] `power_set` — P(A)
- [ ] `find/query by property` — filter sets by property values
- [ ] `calculated_set` - Have sets tyhat contain a test and accepts as a member of the set any property that fits the qualification

## Consistency cleanup
- [ ] `create_set` error message should include the duplicate name
- [x] `is_subset`/`is_superset`/`is_equal`/`is_proper_subset` should set `EC`/`error_message` on not-found

## Query Language Features

### Implemented
- [x] `GET ELEMENTS OF` / `GET SETS OF` / `GET ALL SETS` / `GET CACHE SETS`
- [x] `IS` queries (ELEMENT, SET, SUBSET, SUPERSET)
- [x] Set algebra operators (UNION, INTERSECTION, DIFFERENCE, SYMDIFF)
- [x] Dot notation precedence
- [x] `CREATE SETS`
- [x] `TRASH SETS`
- [x] `WHERE` filtering (>, <, =, >=, <=)

### Membership
- [x] `ADD <Element> TO <Set>` — expose `add_member`
- [x] `REMOVE <Element> FROM <Set>` — expose `remove_member`

### Properties
- [ ] `SET <Set> <key> = <value>` — expose `add_property`
- [ ] `GET PROPERTY <key> OF <Set>` — expose `get_property`
- [x] `UPDATE <Set> <key> = <value>` — expose `update_property`
- [ ] `DELETE PROPERTY <key> OF <Set>` — expose `delete_property`
- [ ] `GET PROPERTIES OF <Set>` — expose `list_properties`
- [ ] `GET KEYS OF <Set>` — expose `list_property_keys`

### IS queries (missing)
- [ ] `IS <Set> DISJOINT <Set>` — expose `is_disjoint`
- [ ] `IS <Set> EQUAL <Set>` — expose `is_equal`
- [ ] `IS <Set> PROPER SUBSET OF <Set>` — expose `is_proper_subset`

### Set algebra (missing)
- [ ] `COMPLEMENT OF <Set>` — expose `set_complement`

### Utility
- [x] `GET CARDINALITY OF <Set>` — expose `get_cardinality`
- [ ] `RENAME <Set> TO <NewName>` — expose `rename_set`
- [ ] `CLEAR CACHE` — expose `clear_cache`
- [ ] `SAVE <path>` / `LOAD <path>` — expose `save_cantordb` / `load_cantordb`

### Aliases
- [ ] `ALIAS <name> AS <Set>` — create a shorthand name for a set or expression
- [ ] Resolve aliases during lexing/collapse so they work transparently in any query

### Deleted set rollback
- [ ] `RESTORE <Set>` — recover a trashed set from `__trash__` back into the database
- [ ] `GET TRASH SETS` — list sets currently in the trash

### Logical operators
- [ ] AND/OR support in WHERE clauses (e.g. `WHERE age > 18 AND name = Bob`)
