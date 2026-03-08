# cantorDB TODO

## Essential (needed before query language)
- [ ] `get_property` — read properties back through the API
- [ ] `delete_property` — remove a key from a set
- [ ] `update_property` — modify an existing key's value
- [ ] `list_members` — enumerate a set's elements
- [ ] `get_cardinality` — return element count of a set
- [ ] `list_sets` — enumerate all sets in the db
- [ ] `rename_set`

## Nice to have (depends on query language scope)
- [ ] `cartesian_product` — A × B
- [ ] `power_set` — P(A)
- [ ] `find/query by property` — filter sets by property values

## Consistency cleanup
- [ ] `create_set` error message should include the duplicate name
- [ ] `is_subset`/`is_superset`/`is_equal`/`is_proper_subset` should set `EC`/`error_message` on not-found
