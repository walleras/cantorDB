#' Create a new CantorDB database
#' @param name Database name
#' @return A cantordb object
#' @export
cdb_new <- function(name) {
  obj <- list(ptr = rcdb_new(name))
  class(obj) <- "cantordb"
  obj
}

#' Load a CantorDB database from file
#' @param path Path to .cdb file
#' @return A cantordb object
#' @export
cdb_load <- function(path) {
  obj <- list(ptr = rcdb_load(path))
  class(obj) <- "cantordb"
  obj
}

# Internal helper to extract the pointer
.ptr <- function(db) db$ptr

#' Save a CantorDB database to file
#' @param db A cantordb object
#' @param path Output file path
#' @return TRUE on success
#' @export
cdb_save <- function(db, path) {
  rcdb_save(.ptr(db), path)
}

#' Print summary of a CantorDB database
#' @param x A cantordb object
#' @param ... Ignored
#' @export
print.cantordb <- function(x, ...) {
  name <- rcdb_name(.ptr(x))
  sets <- rcdb_list_all_sets(.ptr(x))
  cat(sprintf("CantorDB: %s\n", name))
  cat(sprintf("Sets: %d\n", length(sets)))
  if (length(sets) > 0 && length(sets) <= 20) {
    cat("  ", paste(sets, collapse = ", "), "\n")
  } else if (length(sets) > 20) {
    cat("  ", paste(sets[1:20], collapse = ", "), ", ...\n")
  }
  invisible(x)
}

#' Create a new set
#' @export
cdb_create_set <- function(db, set_name) {
  rcdb_create_set(.ptr(db), set_name)
}

#' Delete a set
#' @export
cdb_delete_set <- function(db, set_name) {
  rcdb_delete_set(.ptr(db), set_name)
}

#' Rename a set
#' @export
cdb_rename_set <- function(db, set_name, new_name) {
  rcdb_rename_set(.ptr(db), set_name, new_name)
}

#' Add an element to a set
#' @export
cdb_add_member <- function(db, set_name, element_name) {
  rcdb_add_member(.ptr(db), set_name, element_name)
}

#' Remove an element from a set
#' @export
cdb_remove_member <- function(db, set_name, element_name) {
  rcdb_remove_member(.ptr(db), set_name, element_name)
}

#' Add a property to a set
#' @param db A cantordb object
#' @param set_name Name of the set
#' @param key Property key name
#' @param value Property value (auto-detects type: integer, numeric, logical, character)
#' @export
cdb_add_property <- function(db, set_name, key, value) {
  p <- .ptr(db)
  if (is.logical(value)) {
    rcdb_add_property_bool(p, key, value, set_name)
  } else if (is.integer(value)) {
    rcdb_add_property_int(p, key, value, set_name)
  } else if (is.numeric(value)) {
    rcdb_add_property_double(p, key, value, set_name)
  } else {
    rcdb_add_property_string(p, key, as.character(value), set_name)
  }
}

#' Delete a property from a set
#' @export
cdb_delete_property <- function(db, set_name, key) {
  rcdb_delete_property(.ptr(db), set_name, key)
}

#' Update a property on a set
#' @export
cdb_update_property <- function(db, set_name, key, value) {
  p <- .ptr(db)
  if (is.logical(value)) {
    rcdb_update_property_bool(p, set_name, key, value)
  } else if (is.integer(value)) {
    rcdb_update_property_int(p, set_name, key, value)
  } else if (is.numeric(value)) {
    rcdb_update_property_double(p, set_name, key, value)
  } else {
    rcdb_update_property_string(p, set_name, key, as.character(value))
  }
}

#' Get a single property value
#' @param db A cantordb object
#' @param set_name Set name
#' @param key Property key
#' @return The property value (typed), or NULL if not found
#' @export
cdb_get_property <- function(db, set_name, key) {
  result <- rcdb_get_property(.ptr(db), set_name, key)
  result$value
}

#' List elements of a set
#' @return Character vector of element names
#' @export
cdb_list_elements <- function(db, set_name) {
  rcdb_list_elements(.ptr(db), set_name)
}

#' List all user-created sets
#' @return Character vector of set names
#' @export
cdb_list_all_sets <- function(db) {
  rcdb_list_all_sets(.ptr(db))
}

#' List cached (computed) sets
#' @return Character vector of cached set names
#' @export
cdb_list_cached_sets <- function(db) {
  rcdb_list_cached_sets(.ptr(db))
}

#' List all properties of a set as a data frame
#' @return Data frame with columns: key, value, type
#' @export
cdb_list_properties <- function(db, set_name) {
  rcdb_list_properties(.ptr(db), set_name)
}

#' List property keys and their types
#' @return Data frame with columns: key, type
#' @export
cdb_list_property_keys <- function(db, set_name) {
  rcdb_list_property_keys(.ptr(db), set_name)
}

#' List string properties
#' @export
cdb_list_string_properties <- function(db, set_name) {
  rcdb_list_typed_properties(.ptr(db), set_name, "string")
}

#' List integer properties
#' @export
cdb_list_int_properties <- function(db, set_name) {
  rcdb_list_typed_properties(.ptr(db), set_name, "int")
}

#' List double properties
#' @export
cdb_list_double_properties <- function(db, set_name) {
  rcdb_list_typed_properties(.ptr(db), set_name, "double")
}

#' List boolean properties
#' @export
cdb_list_bool_properties <- function(db, set_name) {
  rcdb_list_typed_properties(.ptr(db), set_name, "bool")
}

#' List long properties
#' @export
cdb_list_long_properties <- function(db, set_name) {
  rcdb_list_typed_properties(.ptr(db), set_name, "long")
}

#' Get the number of elements in a set
#' @export
cdb_get_cardinality <- function(db, set_name) {
  rcdb_get_cardinality(.ptr(db), set_name)
}

#' Set union (A U B)
#' @return Name of the cached result set
#' @export
cdb_set_union <- function(db, a, b) {
  rcdb_set_union(.ptr(db), a, b)
}

#' Set intersection (A n B)
#' @return Name of the cached result set
#' @export
cdb_set_intersection <- function(db, a, b) {
  rcdb_set_intersection(.ptr(db), a, b)
}

#' Set difference (A - B)
#' @return Name of the cached result set
#' @export
cdb_set_difference <- function(db, a, b) {
  rcdb_set_difference(.ptr(db), a, b)
}

#' Symmetric difference (A delta B)
#' @return Name of the cached result set
#' @export
cdb_symmetric_difference <- function(db, a, b) {
  rcdb_symmetric_difference(.ptr(db), a, b)
}

#' Set complement (U - A)
#' @return Name of the cached result set
#' @export
cdb_set_complement <- function(db, set_name) {
  rcdb_set_complement(.ptr(db), set_name)
}

#' Test if A is a subset of B
#' @export
cdb_is_subset <- function(db, a, b) {
  rcdb_is_subset(.ptr(db), a, b)
}

#' Test if A is a superset of B
#' @export
cdb_is_superset <- function(db, a, b) {
  rcdb_is_superset(.ptr(db), a, b)
}

#' Test if A and B are disjoint
#' @export
cdb_is_disjoint <- function(db, a, b) {
  rcdb_is_disjoint(.ptr(db), a, b)
}

#' Test if A and B have the same elements
#' @export
cdb_is_equal <- function(db, a, b) {
  rcdb_is_equal(.ptr(db), a, b)
}

#' Clear all cached (computed) sets
#' @export
cdb_clear_cache <- function(db) {
  rcdb_clear_cache(.ptr(db))
}
