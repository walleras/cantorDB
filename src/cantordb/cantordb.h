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

using namespace std;

extern const string UNIVERSAL_SET;
extern const string CACHE_SET;

enum ERROR_CODE {ER_SET_EX, ER_SET_NOT_FOUND, ER_KEY_EX, ER_ELEM_NOT_MEM};

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
	ERROR_CODE EC;
	string error_message;


	cantordb(string name);
	~cantordb();

	bool create_set(string set_name);
	bool add_property(string key, int value, string set_name);
	bool add_property(string key, string value, string set_name);
	bool add_property(string key, double value, string set_name);
	bool add_property(string key, bool value, string set_name);
	bool add_property(string key, long value, string set_name);
	bool delete_set(string set_name);
	bool add_member(string set_name, string element_name);
	void add_member(string set_name, Set* element);
	bool remove_member(string set_name, string element_name);
	Set* set_union(string set_a_name, string set_b_name);
	Set* set_intersection(string set_a_name, string set_b_name);
	Set* set_difference(string set_a_name, string set_b_name);
	bool is_disjoint(string set_a_name, string set_b_name);
	Set* symmetric_difference(string set_a_name, string set_b_name);
	Set* set_complement(string set_name);
	bool is_subset(string set_a_name, string set_b_name);
	bool is_superset(string set_a_name, string set_b_name);
	bool is_equal(string set_a_name, string set_b_name);
	bool is_proper_subset(string set_a_name, string set_b_name);
};

bool save_cantordb(const cantordb& db, const string& path);
cantordb* load_cantordb(const string& path);

#endif
