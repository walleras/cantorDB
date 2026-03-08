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
- [ ] `is_subset`/`is_superset`/`is_equal`/`is_proper_subset` should set `EC`/`error_message` on not-found
