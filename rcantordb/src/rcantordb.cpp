#include <Rcpp.h>
#include "cantordb.h"

using namespace Rcpp;

// [[Rcpp::export]]
SEXP rcdb_new(std::string name) {
	cantordb* db = new cantordb(name);
	XPtr<cantordb> ptr(db, true);
	return ptr;
}

// [[Rcpp::export]]
SEXP rcdb_load(std::string path) {
	cantordb* db = load_cantordb(path);
	if (!db) stop("Failed to load database from: " + path);
	XPtr<cantordb> ptr(db, true);
	return ptr;
}

// [[Rcpp::export]]
bool rcdb_save(SEXP db_ptr, std::string path) {
	XPtr<cantordb> db(db_ptr);
	return save_cantordb(*db, path);
}

// [[Rcpp::export]]
bool rcdb_create_set(SEXP db_ptr, std::string set_name) {
	XPtr<cantordb> db(db_ptr);
	bool ok = db->create_set(set_name);
	if (!ok) warning(db->error_message);
	return ok;
}

// [[Rcpp::export]]
bool rcdb_delete_set(SEXP db_ptr, std::string set_name) {
	XPtr<cantordb> db(db_ptr);
	bool ok = db->delete_set(set_name);
	if (!ok) warning(db->error_message);
	return ok;
}

// [[Rcpp::export]]
bool rcdb_rename_set(SEXP db_ptr, std::string set_name, std::string new_name) {
	XPtr<cantordb> db(db_ptr);
	bool ok = db->rename_set(set_name, new_name);
	if (!ok) warning(db->error_message);
	return ok;
}

// [[Rcpp::export]]
bool rcdb_add_member(SEXP db_ptr, std::string set_name, std::string element_name) {
	XPtr<cantordb> db(db_ptr);
	bool ok = db->add_member(set_name, element_name);
	if (!ok) warning(db->error_message);
	return ok;
}

// [[Rcpp::export]]
bool rcdb_remove_member(SEXP db_ptr, std::string set_name, std::string element_name) {
	XPtr<cantordb> db(db_ptr);
	bool ok = db->remove_member(set_name, element_name);
	if (!ok) warning(db->error_message);
	return ok;
}

// [[Rcpp::export]]
bool rcdb_add_property_int(SEXP db_ptr, std::string key, int value, std::string set_name) {
	XPtr<cantordb> db(db_ptr);
	bool ok = db->add_property(key, value, set_name);
	if (!ok) warning(db->error_message);
	return ok;
}

// [[Rcpp::export]]
bool rcdb_add_property_string(SEXP db_ptr, std::string key, std::string value, std::string set_name) {
	XPtr<cantordb> db(db_ptr);
	bool ok = db->add_property(key, value, set_name);
	if (!ok) warning(db->error_message);
	return ok;
}

// [[Rcpp::export]]
bool rcdb_add_property_double(SEXP db_ptr, std::string key, double value, std::string set_name) {
	XPtr<cantordb> db(db_ptr);
	bool ok = db->add_property(key, value, set_name);
	if (!ok) warning(db->error_message);
	return ok;
}

// [[Rcpp::export]]
bool rcdb_add_property_bool(SEXP db_ptr, std::string key, bool value, std::string set_name) {
	XPtr<cantordb> db(db_ptr);
	bool ok = db->add_property(key, value, set_name);
	if (!ok) warning(db->error_message);
	return ok;
}

// [[Rcpp::export]]
bool rcdb_delete_property(SEXP db_ptr, std::string set_name, std::string key_name) {
	XPtr<cantordb> db(db_ptr);
	bool ok = db->delete_property(set_name, key_name);
	if (!ok) warning(db->error_message);
	return ok;
}

// [[Rcpp::export]]
bool rcdb_update_property_int(SEXP db_ptr, std::string set_name, std::string key, int value) {
	XPtr<cantordb> db(db_ptr);
	bool ok = db->update_property(set_name, key, value);
	if (!ok) warning(db->error_message);
	return ok;
}

// [[Rcpp::export]]
bool rcdb_update_property_string(SEXP db_ptr, std::string set_name, std::string key, std::string value) {
	XPtr<cantordb> db(db_ptr);
	bool ok = db->update_property(set_name, key, value);
	if (!ok) warning(db->error_message);
	return ok;
}

// [[Rcpp::export]]
bool rcdb_update_property_double(SEXP db_ptr, std::string set_name, std::string key, double value) {
	XPtr<cantordb> db(db_ptr);
	bool ok = db->update_property(set_name, key, value);
	if (!ok) warning(db->error_message);
	return ok;
}

// [[Rcpp::export]]
bool rcdb_update_property_bool(SEXP db_ptr, std::string set_name, std::string key, bool value) {
	XPtr<cantordb> db(db_ptr);
	bool ok = db->update_property(set_name, key, value);
	if (!ok) warning(db->error_message);
	return ok;
}

// [[Rcpp::export]]
List rcdb_get_property(SEXP db_ptr, std::string set_name, std::string key_name) {
	XPtr<cantordb> db(db_ptr);
	auto s = db->set_index.find(set_name);
	if (s == db->set_index.end()) {
		warning("Set not found: " + set_name);
		return List::create(Named("value") = R_NilValue, Named("type") = "error");
	}
	Set* set = s->second;
	if (set->key_num.count(key_name))
		return List::create(Named("value") = set->key_num[key_name], Named("type") = "int");
	if (set->key_value.count(key_name))
		return List::create(Named("value") = set->key_value[key_name], Named("type") = "string");
	if (set->key_decimal.count(key_name))
		return List::create(Named("value") = set->key_decimal[key_name], Named("type") = "double");
	if (set->key_bool.count(key_name))
		return List::create(Named("value") = set->key_bool[key_name], Named("type") = "bool");
	if (set->key_long.count(key_name))
		return List::create(Named("value") = (double)set->key_long[key_name], Named("type") = "long");
	warning("Key not found: " + key_name);
	return List::create(Named("value") = R_NilValue, Named("type") = "error");
}

// [[Rcpp::export]]
CharacterVector rcdb_list_elements(SEXP db_ptr, std::string set_name) {
	XPtr<cantordb> db(db_ptr);
	std::string raw = db->list_elements(set_name);
	if (raw.empty()) return CharacterVector();
	CharacterVector result;
	std::istringstream ss(raw);
	std::string line;
	while (std::getline(ss, line)) result.push_back(line);
	return result;
}

// [[Rcpp::export]]
CharacterVector rcdb_list_all_sets(SEXP db_ptr) {
	XPtr<cantordb> db(db_ptr);
	std::string raw = db->list_all_sets();
	if (raw.empty()) return CharacterVector();
	CharacterVector result;
	std::istringstream ss(raw);
	std::string line;
	while (std::getline(ss, line)) result.push_back(line);
	return result;
}

// [[Rcpp::export]]
CharacterVector rcdb_list_cached_sets(SEXP db_ptr) {
	XPtr<cantordb> db(db_ptr);
	std::string raw = db->list_cached_sets();
	if (raw.empty()) return CharacterVector();
	CharacterVector result;
	std::istringstream ss(raw);
	std::string line;
	while (std::getline(ss, line)) result.push_back(line);
	return result;
}

// [[Rcpp::export]]
DataFrame rcdb_list_properties(SEXP db_ptr, std::string set_name) {
	XPtr<cantordb> db(db_ptr);
	std::string raw = db->list_properties(set_name);
	if (raw.empty()) return DataFrame();
	CharacterVector keys, values, types;
	std::istringstream ss(raw);
	std::string line;
	while (std::getline(ss, line)) {
		size_t p1 = line.find(':');
		size_t p2 = line.rfind(':');
		if (p1 != std::string::npos && p2 != p1) {
			keys.push_back(line.substr(0, p1));
			values.push_back(line.substr(p1 + 1, p2 - p1 - 1));
			types.push_back(line.substr(p2 + 1));
		}
	}
	return DataFrame::create(Named("key") = keys, Named("value") = values,
	                         Named("type") = types, Named("stringsAsFactors") = false);
}

// [[Rcpp::export]]
DataFrame rcdb_list_property_keys(SEXP db_ptr, std::string set_name) {
	XPtr<cantordb> db(db_ptr);
	std::string raw = db->list_property_keys(set_name);
	if (raw.empty()) return DataFrame();
	CharacterVector keys, types;
	std::istringstream ss(raw);
	std::string line;
	while (std::getline(ss, line)) {
		size_t p = line.find(": ");
		if (p != std::string::npos) {
			keys.push_back(line.substr(0, p));
			types.push_back(line.substr(p + 2));
		}
	}
	return DataFrame::create(Named("key") = keys, Named("type") = types,
	                         Named("stringsAsFactors") = false);
}

// [[Rcpp::export]]
DataFrame rcdb_list_typed_properties(SEXP db_ptr, std::string set_name, std::string type) {
	XPtr<cantordb> db(db_ptr);
	std::string raw;
	if (type == "string")      raw = db->list_string_properties(set_name);
	else if (type == "int")    raw = db->list_int_properties(set_name);
	else if (type == "double") raw = db->list_double_properties(set_name);
	else if (type == "bool")   raw = db->list_bool_properties(set_name);
	else if (type == "long")   raw = db->list_long_properties(set_name);
	else { warning("Unknown type: " + type); return DataFrame(); }

	if (raw.empty()) return DataFrame();
	CharacterVector keys, values;
	std::istringstream ss(raw);
	std::string line;
	while (std::getline(ss, line)) {
		size_t p = line.find(':');
		if (p != std::string::npos) {
			keys.push_back(line.substr(0, p));
			values.push_back(line.substr(p + 1));
		}
	}
	return DataFrame::create(Named("key") = keys, Named("value") = values,
	                         Named("stringsAsFactors") = false);
}

// [[Rcpp::export]]
int rcdb_get_cardinality(SEXP db_ptr, std::string set_name) {
	XPtr<cantordb> db(db_ptr);
	return db->get_cardinality(set_name);
}

// [[Rcpp::export]]
SEXP rcdb_set_union(SEXP db_ptr, std::string a, std::string b) {
	XPtr<cantordb> db(db_ptr);
	Set* result = db->set_union(a, b);
	if (!result) { warning(db->error_message); return R_NilValue; }
	return wrap(result->set_name);
}

// [[Rcpp::export]]
SEXP rcdb_set_intersection(SEXP db_ptr, std::string a, std::string b) {
	XPtr<cantordb> db(db_ptr);
	Set* result = db->set_intersection(a, b);
	if (!result) { warning(db->error_message); return R_NilValue; }
	return wrap(result->set_name);
}

// [[Rcpp::export]]
SEXP rcdb_set_difference(SEXP db_ptr, std::string a, std::string b) {
	XPtr<cantordb> db(db_ptr);
	Set* result = db->set_difference(a, b);
	if (!result) { warning(db->error_message); return R_NilValue; }
	return wrap(result->set_name);
}

// [[Rcpp::export]]
SEXP rcdb_symmetric_difference(SEXP db_ptr, std::string a, std::string b) {
	XPtr<cantordb> db(db_ptr);
	Set* result = db->symmetric_difference(a, b);
	if (!result) { warning(db->error_message); return R_NilValue; }
	return wrap(result->set_name);
}

// [[Rcpp::export]]
SEXP rcdb_set_complement(SEXP db_ptr, std::string set_name) {
	XPtr<cantordb> db(db_ptr);
	Set* result = db->set_complement(set_name);
	if (!result) { warning(db->error_message); return R_NilValue; }
	return wrap(result->set_name);
}

// [[Rcpp::export]]
bool rcdb_is_subset(SEXP db_ptr, std::string a, std::string b) {
	XPtr<cantordb> db(db_ptr);
	return db->is_subset(a, b);
}

// [[Rcpp::export]]
bool rcdb_is_superset(SEXP db_ptr, std::string a, std::string b) {
	XPtr<cantordb> db(db_ptr);
	return db->is_superset(a, b);
}

// [[Rcpp::export]]
bool rcdb_is_disjoint(SEXP db_ptr, std::string a, std::string b) {
	XPtr<cantordb> db(db_ptr);
	return db->is_disjoint(a, b);
}

// [[Rcpp::export]]
bool rcdb_is_equal(SEXP db_ptr, std::string a, std::string b) {
	XPtr<cantordb> db(db_ptr);
	return db->is_equal(a, b);
}

// [[Rcpp::export]]
bool rcdb_clear_cache(SEXP db_ptr) {
	XPtr<cantordb> db(db_ptr);
	return db->clear_cache();
}

// [[Rcpp::export]]
std::string rcdb_name(SEXP db_ptr) {
	XPtr<cantordb> db(db_ptr);
	return db->name;
}
