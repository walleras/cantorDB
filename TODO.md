# cantorDB Roadmap

## Near future
- [ ] WAL support
- [ ] `BEGIN/COMMIT` atomic transaction support
- [ ] Cross-property WHERE comparisons (`WHERE prop_a > prop_b`)
- [ ] Mutation success messages — return confirmation on CREATE, ADD, REMOVE, UPDATE, etc.
- [x] Logical operators
- [x] `FILTER` support
- [x] `RESTORE` support


## Far future
- [ ] CSV import/export (`EXPORT <set algebra> TO file.csv`, `IMPORT file.csv TO <Set>`)
- [ ] Server
- [ ] Multi Users/Authentication
- [ ] `ALIAS <name> AS <Set>` — create a shorthand name for a set or expression
- [ ] Multithreading/Queing
- [ ] Views
- [ ] `ORDER` support

## Planned but no road map
- [ ] Python bindings
- [ ] R bindings
- [ ] Walking up and down multiple levels of heirarchy


## Maybe
- [ ] Primitive sets
- [ ] Logical gate sets
- [ ] `power_set` — P(A)
- [ ] `find/query by property` — filter sets by property values // This kind of already exists...
- [ ] Storage of documents and files within set properties

## Consistency cleanup
- [ ] `create_set` error message should include the duplicate name

## Never
- [ ] On disk persistance
- [ ] `cartesian_product` — A × B
