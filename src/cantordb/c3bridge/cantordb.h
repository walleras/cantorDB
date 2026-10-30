#ifndef CANTORDB_C3_SHIM_H
#define CANTORDB_C3_SHIM_H

// Drop-in stand-in for the C++ cantordb class, backed by the C3/Roaring implementation through
// the C ABI in cdb_c.h. Method names and overloads match what lexer/parser/main use, so they
// build against either backend (-DCANTORDB_C3 selects this one).
//
// Set is opaque here: it is the C3 Set*. Use find_set / set_name_of / key_type instead of
// reaching into set_index; property_type replaces the *_property_index maps.

#include <string>
#include <variant>
#include <cstdint>
#include <cstddef>

using namespace std;

enum ERROR_CODE {ER_SET_EX, ER_SET_NOT_FOUND, ER_KEY_EX, ER_ELEM_NOT_MEM, ER_ELEM_ALR_ADDED, ER_KEY_NOT_FOUND, ER_KEY_WRONG_TYPE};

enum TYPE{INTEGER, STRING, DOUBLE, BOOL, LONG};

struct Set;
struct cdb_db;

class cantordb {
public:
	string name;
	ERROR_CODE EC;
	string error_message;

	cantordb(string name);
	cantordb(string name, size_t memory_limit_bytes);
	~cantordb();
	cantordb(const cantordb&) = delete;
	cantordb& operator=(const cantordb&) = delete;

	// Replaces this database's contents with the file's. False (and no change) if it can't be read.
	bool load(const string& path);
	bool save(const string& path) const;

	bool create_set(string set_name);
	bool create_set(string set_name, bool gen_set);
	bool delete_set(string set_name, bool safe = true);
	bool restore_set(string set_name);
	bool rename_set(string set_name, string set_new_name);
	bool add_member(string set_name, string element_name);
	bool remove_member(string set_name, string element_name);
	bool clear_cache();

	Set* set_union(string a, string b);
	Set* set_union(Set* a, Set* b);
	Set* set_intersection(string a, string b);
	Set* set_intersection(Set* a, Set* b);
	Set* set_difference(string a, string b);
	Set* set_difference(Set* a, Set* b);
	Set* symmetric_difference(string a, string b);
	Set* symmetric_difference(Set* a, Set* b);
	Set* set_complement(string set_name);

	bool is_element(string a, string b);
	bool is_disjoint(string a, string b);
	bool is_subset(string a, string b);
	bool is_superset(string a, string b);
	bool is_equal(string a, string b);
	bool is_proper_subset(string a, string b);

	bool create_property(string property_name, string type);
	bool delete_property(string set_name, string key_name);
	bool add_property(string property_name, string set_name);
	bool add_property(string key, int value, string set_name);
	bool add_property(string key, string value, string set_name);
	bool add_property(string key, double value, string set_name);
	bool add_property(string key, bool value, string set_name);
	bool add_property(string key, int64_t value, string set_name);
	bool update_property(string set_name, string key_name, int value);
	bool update_property(string set_name, string key_name, string value);
	bool update_property(string set_name, string key_name, double value);
	bool update_property(string set_name, string key_name, bool value);
	bool update_property(string set_name, string key_name, int64_t value);
	variant<bool, int64_t, string, int, double> get_property(string set_name, string key_name);

	Set* where_elements_greater_than(string set_name, string property, int value);
	Set* where_elements_greater_than(string set_name, string property, int64_t value);
	Set* where_elements_greater_than(string set_name, string property, double value);
	Set* where_elements_lesser_than(string set_name, string property, int value);
	Set* where_elements_lesser_than(string set_name, string property, int64_t value);
	Set* where_elements_lesser_than(string set_name, string property, double value);
	Set* where_elements_equal_than(string set_name, string property, int value);
	Set* where_elements_equal_than(string set_name, string property, int64_t value);
	Set* where_elements_equal_than(string set_name, string property, double value);
	Set* where_elements_equal_than(string set_name, string property, string value);
	Set* where_elements_equal_than(string set_name, string property, bool value);

	int get_cardinality(string set_name);
	string list_elements(string set_name);
	string list_sets(string set_name);
	string list_all_sets();
	string list_all_keys();
	string list_property_keys(string set_name);
	string list_properties(string set_name);

	// What the parser used to read straight out of the C++ containers.
	Set* find_set(const string& set_name);               // null if there is no such set
	string set_name_of(Set* s);
	int key_type(const string& set_name, const string& key);   // TYPE of a key on a set, -1 if absent
	int property_type(const string& property_name);            // TYPE a property was created with, -1 if unknown

private:
	cdb_db* h;
	struct Call;
	string take(char* p);
	Set* where(const string& set_name, const string& property, int op, int type, int64_t l, double d, const char* s);
	Set* combine(const string& a, const string& b, int op);
	bool relation(const string& a, const string& b, int kind);
	bool add_value(const string& key, int type, int64_t l, double d, const char* s, const string& set_name);
	bool update_value(const string& set_name, const string& key, int type, int64_t l, double d, const char* s);
	friend bool save_cantordb(const cantordb& db, const string& path);
	friend cantordb* load_cantordb(const string& path);
	cantordb();
};

bool save_cantordb(const cantordb& db, const string& path);
cantordb* load_cantordb(const string& path);

#endif
