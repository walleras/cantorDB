# C3 backend for the C++ shell

The SeQL lexer, parser and shell (`../lexer.*`, `../parser.*`, `../main.cpp`) can run on either
database implementation:

| backend | build | database |
|---|---|---|
| C++ (default) | CMake / `g++ main.cpp lexer.cpp parser.cpp cantordb.cpp` | `../cantordb.cpp` |
| C3 + Roaring | `./build.ps1` (defines `CANTORDB_C3`) | `../rewrite/*.c3` through the C ABI |

## How it fits together

- `../rewrite/capi.c3` exports a C ABI (`cdb_*`), declared in `cdb_c.h`. Strings in are NUL-terminated;
  strings out are heap copies the shim frees; sets are opaque handles; property values travel as
  `(type tag, int64, double, char*)`.
- `cantordb.h` / `cantordb.cpp` are a drop-in `cantordb` class with the method names and overloads the
  parser uses, forwarding to the C ABI. `error_message` is filled from the C3 error after any call that raised one.
- The parser used to read C++ containers directly (`set_index`, `*_property_index`, `Set::key_index`,
  `Set::set_name`) and swapped members on `LOAD`. Those spots now go through five small helpers at the
  top of `parser.cpp` (`find_set_`, `set_name_of_`, `key_type_`, `property_type_`, `load_into_`) that
  have one implementation per backend.

## Known differences from the C++ backend

- Listings of property keys are sorted by name (C++ lists them in hash order).
- `.cdb` files load in both directions (the C3 reader/writer uses the C++ file layout).
- CRoaring is built with gcc and linked in directly; the project's own `c-sources` build of
  `roaring.c` is not used for the bridge target.
