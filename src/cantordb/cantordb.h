#ifndef CANTORDB_H
#define CANTORDB_H

#include <string>
#include <map>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <cstdint>
#include <cstdio>
#include <variant>

using namespace std;

extern const string UNIVERSAL_SET;
extern const string CACHE_SET;
extern const string TRASH_SET;

enum ERROR_CODE {ER_SET_EX, ER_SET_NOT_FOUND, ER_KEY_EX, ER_ELEM_NOT_MEM, ER_KEY_NOT_FOUND, ER_KEY_WRONG_TYPE};

enum TYPE{INTEGER, STRING, DOUBLE, BOOL, LONG};

struct Set {
	string set_name;
	unordered_map<string, int> key_index;
	map<string, int> key_num;
	map<string, string> key_value;
	map<string, double> key_decimal;
	map<string, bool> key_bool;
	map<string, long> key_long;
	vector<Set*> member_of;
	vector<Set*> has_element;
};

class cantordb {
public:
	string name;
	map<string, Set*> set_index;
	unordered_map<string, vector<Set*>> integer_property_index;
	unordered_map<string, vector<Set*>> string_property_index;
	unordered_map<string, vector<Set*>> double_property_index;
	unordered_map<string, vector<Set*>> bool_property_index;
	unordered_map<string, vector<Set*>> long_property_index;
	unordered_map<string, TYPE> property_types;
	ERROR_CODE EC;
	string error_message;
	bool emergency_shut_off;
	size_t memory_used;
	size_t memory_limit;

	cantordb(string name);
	cantordb(string name, size_t memory_limit_bytes);
	~cantordb();

	bool create_set(string set_name);
	bool create_set(string set_name, bool gen_set);
	bool add_property(string key, int value, string set_name);
	bool add_property(string key, string value, string set_name);
	bool add_property(string key, double value, string set_name);
	bool add_property(string key, bool value, string set_name);
	bool add_property(string key, long value, string set_name);
	bool delete_set(string set_name, bool safe = true);
	bool add_member(string set_name, string element_name);
	void add_member(string set_name, Set* element);
	bool remove_member(string set_name, string element_name);
	Set* set_union(string set_a_name, string set_b_name);
	Set* set_union(Set* set_a, Set* set_b);
	Set* set_intersection(string set_a_name, string set_b_name);
	Set* set_intersection(Set* set_a, Set* set_b);
	Set* set_difference(string set_a_name, string set_b_name);
	Set* set_difference(Set* set_a, Set* set_b);
	bool is_disjoint(string set_a_name, string set_b_name);
	bool is_disjoint(Set* set_a, Set* set_b);
	Set* symmetric_difference(string set_a_name, string set_b_name);
	Set* symmetric_difference(Set* set_a, Set* set_b);
	Set* set_complement(string set_name);
	bool is_subset(string set_a_name, string set_b_name);
	bool is_subset(Set* set_a, Set* set_b);
	bool is_superset(string set_a_name, string set_b_name);
	bool is_superset(Set* set_a, Set* set_b);
	bool is_equal(string set_a_name, string set_b_name);
	bool is_equal(Set* set_a, Set* set_b);
	bool is_proper_subset(string set_a_name, string set_b_name);
	bool is_proper_subset(Set* set_a, Set* set_b);
	variant<bool, long, string, int, double> get_property(string set_name, string key_name);
	int get_property_safe_int(string set_name, string key_name);
	string get_property_safe_string(string set_name, string key_name);
	double get_property_safe_double(string set_name, string key_name);
	bool get_property_safe_bool(string set_name, string key_name);
	long get_property_safe_long(string set_name, string key_name);
	bool create_property(string property_name, string type);
	bool add_property(string property_name, string set_name);
	bool delete_property(string set_name, string key_name);
	bool update_property(string set_name, string key_name, int value);
	bool update_property(string set_name, string key_name, string value);
	bool update_property(string set_name, string key_name, double value);
	bool update_property(string set_name, string key_name, bool value);
	bool update_property(string set_name, string key_name, long value);
	string list_property_keys(string set_name);
	string list_properties(string set_name);
	string list_string_properties(string set_name);
	string list_int_properties(string set_name);
	string list_double_properties(string set_name);
	string list_bool_properties(string set_name);
	string list_long_properties(string set_name);
	string list_elements(string set_name);
	string list_sets(string set_name);
	int get_cardinality(string set_name);
	string list_all_sets();
	string list_cached_sets();
	bool clear_cache();
	bool rename_set(string set_name, string set_new_name);
	bool is_element(string set_a_name, string set_b_name);
	Set* where_elements_greater_than(string set_name, string property, int value);
	Set* where_elements_greater_than(string set_name, string property, long value);
	Set* where_elements_greater_than(string set_name, string property, double value);
	Set* where_elements_lesser_than(string set_name, string property, int value);
	Set* where_elements_lesser_than(string set_name, string property, long value);
	Set* where_elements_lesser_than(string set_name, string property, double value);
	Set* where_elements_equal_than(string set_name, string property, int value);
	Set* where_elements_equal_than(string set_name, string property, long value);
	Set* where_elements_equal_than(string set_name, string property, double value);
	Set* where_elements_equal_than(string set_name, string property, string value);
	Set* where_elements_equal_than(string set_name, string property, bool value);
};

bool save_cantordb(const cantordb& db, const string& path);
cantordb* load_cantordb(const string& path);

#endif
