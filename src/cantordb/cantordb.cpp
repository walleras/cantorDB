#include "cantordb.h"
#include <algorithm>
#include <cassert>
#include <new>

const string UNIVERSAL_SET = "__universal__";
const string CACHE_SET = "__cache__";
const string TRASH_SET = "__trash__";

static const size_t SET_BASE_COST = sizeof(Set) + 64;

cantordb::cantordb(string name) {
	this->name = name;
	this->emergency_shut_off = false;
	this->memory_used = 0;
	this->memory_limit = 0;
	create_set(UNIVERSAL_SET, true);
	create_set(CACHE_SET, true);
	create_set(TRASH_SET, true);
}

cantordb::cantordb(string name, size_t memory_limit_bytes) {
	this->name = name;
	this->emergency_shut_off = false;
	this->memory_used = 0;
	this->memory_limit = memory_limit_bytes;
	create_set(UNIVERSAL_SET, true);
	create_set(CACHE_SET, true);
	create_set(TRASH_SET, true);
}

cantordb::~cantordb() {
	// Two-pass shutdown: break all cross-references before freeing memory.
	// Pass 1: clear membership vectors so no Set* dangles during deletion
	for(auto& [name, s] : set_index) {
		s->member_of.clear();
		s->has_element.clear();
	}
	// Clear property indices (also hold Set* pointers into deleted Sets)
	integer_property_index.clear();
	string_property_index.clear();
	double_property_index.clear();
	bool_property_index.clear();
	long_property_index.clear();
	// Pass 2: delete all Set objects
	for(auto& [name, s] : set_index) {
		delete s;
	}
	set_index.clear();
}

bool cantordb::create_set(string set_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) != set_index.end()) {
		EC = ER_SET_EX;
		error_message = "Error: Set name already exists. Use a unique name for every set.";

		return false;
	}
	Set* s = new (nothrow) Set();
	if(!s) { emergency_shut_off = true; error_message = "Error: Out of memory."; return false; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete s; return false; }
	s->set_name = set_name;
	set_index[set_name] = s;
	add_member(UNIVERSAL_SET, set_name);
	return true;
}


bool cantordb::create_set(string set_name, bool gen_set) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) != set_index.end()) {
		EC = ER_SET_EX;
		error_message = "Error: Set name already exists. Use a unique name for every set.";

		return false;
	}
	Set* s = new (nothrow) Set();
	if(!s) { emergency_shut_off = true; error_message = "Error: Out of memory."; return false; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete s; return false; }
	s->set_name = set_name;
	set_index[set_name] = s;
	if(!gen_set) {
		add_member(UNIVERSAL_SET, set_name);
	}
	return true;
}

bool cantordb::create_property(string property_name, string type) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(property_types.find(property_name) != property_types.end()) {
		EC = ER_KEY_EX;
		error_message = "Error: Property \"" + property_name + "\" already registered.";
		return false;
	}
	if(type == "int" || type == "integer") {
		property_types[property_name] = INTEGER;
	} else if(type == "string") {
		property_types[property_name] = STRING;
	} else if(type == "double") {
		property_types[property_name] = DOUBLE;
	} else if(type == "bool") {
		property_types[property_name] = BOOL;
	} else if(type == "long") {
		property_types[property_name] = LONG;
	} else {
		EC = ER_KEY_WRONG_TYPE;
		error_message = "Error: Unknown type \"" + type + "\". Use int, string, double, bool, or long.";
		return false;
	}
	return true;
}

bool cantordb::add_property(string property_name, string set_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(set_index[set_name]->key_index.find(property_name) != set_index[set_name]->key_index.end()) {
		EC = ER_KEY_EX;
		error_message = "Error: Key \"" + property_name + "\" already exists as a property on this set.";
		return false;
	}
	if(property_types.find(property_name) == property_types.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Property \"" + property_name + "\" has not been created. Use create_property first.";
		return false;
	}

	Set* s = set_index[set_name];
	TYPE t = property_types[property_name];

	switch(t) {
		case INTEGER: {
			auto& vec = integer_property_index[property_name];
			auto pos = std::lower_bound(vec.begin(), vec.end(), s);
			vec.insert(pos, s);
			s->key_num[property_name] = 0;
			s->key_index[property_name] = 0;
			break;
		}
		case STRING: {
			auto& vec = string_property_index[property_name];
			auto pos = std::lower_bound(vec.begin(), vec.end(), s);
			vec.insert(pos, s);
			s->key_value[property_name] = "";
			s->key_index[property_name] = 1;
			break;
		}
		case DOUBLE: {
			auto& vec = double_property_index[property_name];
			auto pos = std::lower_bound(vec.begin(), vec.end(), s);
			vec.insert(pos, s);
			s->key_decimal[property_name] = 0.0;
			s->key_index[property_name] = 2;
			break;
		}
		case BOOL: {
			auto& vec = bool_property_index[property_name];
			auto pos = std::lower_bound(vec.begin(), vec.end(), s);
			vec.insert(pos, s);
			s->key_bool[property_name] = false;
			s->key_index[property_name] = 3;
			break;
		}
		case LONG: {
			auto& vec = long_property_index[property_name];
			auto pos = std::lower_bound(vec.begin(), vec.end(), s);
			vec.insert(pos, s);
			s->key_long[property_name] = 0L;
			s->key_index[property_name] = 4;
			break;
		}
	}
	return true;
}

bool cantordb::add_property(string key, int value, string set_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(property_types.find(key) == property_types.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Property \"" + key + "\" has not been created. Use create_property first.";
		return false;
	}
	if(property_types[key] != INTEGER) {
		EC = ER_KEY_WRONG_TYPE;
		error_message = "Error: Property \"" + key + "\" is not int type.";
		return false;
	}
	if(set_index[set_name]->key_index.find(key) != set_index[set_name]->key_index.end()) {
		EC = ER_KEY_EX;
		error_message = "Error: Key \"" + key + "\" already exists as a property.";
		return false;
	}
	{
		auto& vec = integer_property_index[key];
		Set* s = set_index[set_name];
		auto pos = std::lower_bound(vec.begin(), vec.end(), s);
		vec.insert(pos, s);
	}
	set_index[set_name]->key_num[key] = value;
	set_index[set_name]->key_index[key] = 0;
	return true;
}

bool cantordb::add_property(string key, string value, string set_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(property_types.find(key) == property_types.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Property \"" + key + "\" has not been created. Use create_property first.";
		return false;
	}
	if(property_types[key] != STRING) {
		EC = ER_KEY_WRONG_TYPE;
		error_message = "Error: Property \"" + key + "\" is not string type.";
		return false;
	}
	if(set_index[set_name]->key_index.find(key) != set_index[set_name]->key_index.end()) {
		EC = ER_KEY_EX;
		error_message = "Error: Key \"" + key + "\" already exists as a property.";
		return false;
	}
	{
		auto& vec = string_property_index[key];
		Set* s = set_index[set_name];
		auto pos = std::lower_bound(vec.begin(), vec.end(), s);
		vec.insert(pos, s);
	}
	set_index[set_name]->key_value[key] = value;
	set_index[set_name]->key_index[key] = 1;
	memory_used += key.size() + value.size() + 64;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; return false; }
	return true;
}

bool cantordb::add_property(string key, double value, string set_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(property_types.find(key) == property_types.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Property \"" + key + "\" has not been created. Use create_property first.";
		return false;
	}
	if(property_types[key] != DOUBLE) {
		EC = ER_KEY_WRONG_TYPE;
		error_message = "Error: Property \"" + key + "\" is not double type.";
		return false;
	}
	if(set_index[set_name]->key_index.find(key) != set_index[set_name]->key_index.end()) {
		EC = ER_KEY_EX;
		error_message = "Error: Key \"" + key + "\" already exists as a property.";
		return false;
	}
	{
		auto& vec = double_property_index[key];
		Set* s = set_index[set_name];
		auto pos = std::lower_bound(vec.begin(), vec.end(), s);
		vec.insert(pos, s);
	}
	set_index[set_name]->key_decimal[key] = value;
	set_index[set_name]->key_index[key] = 2;
	return true;
}

bool cantordb::add_property(string key, bool value, string set_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(property_types.find(key) == property_types.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Property \"" + key + "\" has not been created. Use create_property first.";
		return false;
	}
	if(property_types[key] != BOOL) {
		EC = ER_KEY_WRONG_TYPE;
		error_message = "Error: Property \"" + key + "\" is not bool type.";
		return false;
	}
	if(set_index[set_name]->key_index.find(key) != set_index[set_name]->key_index.end()) {
		EC = ER_KEY_EX;
		error_message = "Error: Key \"" + key + "\" already exists as a property.";
		return false;
	}
	{
		auto& vec = bool_property_index[key];
		Set* s = set_index[set_name];
		auto pos = std::lower_bound(vec.begin(), vec.end(), s);
		vec.insert(pos, s);
	}
	set_index[set_name]->key_bool[key] = value;
	set_index[set_name]->key_index[key] = 3;
	return true;
}

bool cantordb::add_property(string key, long value, string set_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(property_types.find(key) == property_types.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Property \"" + key + "\" has not been created. Use create_property first.";
		return false;
	}
	if(property_types[key] != LONG) {
		EC = ER_KEY_WRONG_TYPE;
		error_message = "Error: Property \"" + key + "\" is not long type.";
		return false;
	}
	if(set_index[set_name]->key_index.find(key) != set_index[set_name]->key_index.end()) {
		EC = ER_KEY_EX;
		error_message = "Error: Key \"" + key + "\" already exists as a property.";
		return false;
	}
	{
		auto& vec = long_property_index[key];
		Set* s = set_index[set_name];
		auto pos = std::lower_bound(vec.begin(), vec.end(), s);
		vec.insert(pos, s);
	}
	set_index[set_name]->key_long[key] = value;
	set_index[set_name]->key_index[key] = 4;
	return true;
}

bool cantordb::delete_set(string set_name, bool safe) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	Set* target = set_index[set_name];

	// Unlink from all parents' has_element
	for(int i = 0; i < (int)target->member_of.size(); i++) {
		auto& vec = target->member_of[i]->has_element;
		vec.erase(std::remove(vec.begin(), vec.end(), target), vec.end());
	}

	// Unlink from all children's member_of
	for(int i = 0; i < (int)target->has_element.size(); i++) {
		auto& vec = target->has_element[i]->member_of;
		vec.erase(std::remove(vec.begin(), vec.end(), target), vec.end());
	}

	// Remove from property indices
	for(auto& [key, vec] : integer_property_index) {
		auto it = std::lower_bound(vec.begin(), vec.end(), target);
		if(it != vec.end() && *it == target) vec.erase(it);
	}
	for(auto& [key, vec] : string_property_index) {
		auto it = std::lower_bound(vec.begin(), vec.end(), target);
		if(it != vec.end() && *it == target) vec.erase(it);
	}
	for(auto& [key, vec] : double_property_index) {
		auto it = std::lower_bound(vec.begin(), vec.end(), target);
		if(it != vec.end() && *it == target) vec.erase(it);
	}
	for(auto& [key, vec] : bool_property_index) {
		auto it = std::lower_bound(vec.begin(), vec.end(), target);
		if(it != vec.end() && *it == target) vec.erase(it);
	}
	for(auto& [key, vec] : long_property_index) {
		auto it = std::lower_bound(vec.begin(), vec.end(), target);
		if(it != vec.end() && *it == target) vec.erase(it);
	}

	if(safe) {
		// Soft delete: keep the Set* alive in TRASH for possible restore.
		// member_of and has_element on target are preserved as a restore record.
		set_index.erase(set_name);
		Set* trash = set_index[TRASH_SET];
		auto pos = std::lower_bound(trash->has_element.begin(), trash->has_element.end(), target);
		trash->has_element.insert(pos, target);
	} else {
		// Hard delete: destroy the Set* permanently
		delete target;
		set_index.erase(set_name);
	}
	return true;
}

bool cantordb::restore_set(string set_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }

	// Find the set in TRASH by name
	Set* trash = set_index[TRASH_SET];
	Set* target = nullptr;
	for(auto* s : trash->has_element) {
		if(s->set_name == set_name) {
			target = s;
			break;
		}
	}
	if(!target) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found in TRASH.";
		return false;
	}

	// Remove from TRASH
	auto& tvec = trash->has_element;
	tvec.erase(std::remove(tvec.begin(), tvec.end(), target), tvec.end());

	// Re-add to set_index
	set_index[set_name] = target;

	// Re-add to UNIVERSAL
	add_member(UNIVERSAL_SET, set_name);

	// Re-link to all parent sets (member_of was preserved as restore record)
	for(auto* parent : target->member_of) {
		if(parent->set_name == UNIVERSAL_SET) continue; // already re-added above
		auto pos = std::lower_bound(parent->has_element.begin(), parent->has_element.end(), target);
		if(pos == parent->has_element.end() || *pos != target) {
			parent->has_element.insert(pos, target);
		}
	}

	// Re-link to all child sets (has_element was preserved as restore record)
	for(auto* child : target->has_element) {
		auto pos = std::lower_bound(child->member_of.begin(), child->member_of.end(), target);
		if(pos == child->member_of.end() || *pos != target) {
			child->member_of.insert(pos, target);
		}
	}

	// Re-add to property indices
	for(auto& [key, idx] : target->key_index) {
		switch(idx) {
			case 0: { // int
				auto& vec = integer_property_index[key];
				auto p = std::lower_bound(vec.begin(), vec.end(), target);
				if(p == vec.end() || *p != target) vec.insert(p, target);
				break;
			}
			case 1: { // string
				auto& vec = string_property_index[key];
				auto p = std::lower_bound(vec.begin(), vec.end(), target);
				if(p == vec.end() || *p != target) vec.insert(p, target);
				break;
			}
			case 2: { // double
				auto& vec = double_property_index[key];
				auto p = std::lower_bound(vec.begin(), vec.end(), target);
				if(p == vec.end() || *p != target) vec.insert(p, target);
				break;
			}
			case 3: { // bool
				auto& vec = bool_property_index[key];
				auto p = std::lower_bound(vec.begin(), vec.end(), target);
				if(p == vec.end() || *p != target) vec.insert(p, target);
				break;
			}
			case 4: { // long
				auto& vec = long_property_index[key];
				auto p = std::lower_bound(vec.begin(), vec.end(), target);
				if(p == vec.end() || *p != target) vec.insert(p, target);
				break;
			}
		}
	}

	return true;
}

bool cantordb::add_member(string set_name, string element_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(set_index.find(element_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + element_name + "\" not found.";
		return false;
	}
	if(is_element(element_name, set_name)) {
		EC = ER_ELEM_ALR_ADDED;
		error_message = "Error: \"" + element_name + "\" is already an element of \"" + set_name + "\".";
		return false;
	}
	Set* parent = set_index[set_name];
	Set* child = set_index[element_name];
	auto pos = std::lower_bound(parent->has_element.begin(), parent->has_element.end(), child);
	parent->has_element.insert(pos, child);
	auto mpos = std::lower_bound(child->member_of.begin(), child->member_of.end(), parent);
	child->member_of.insert(mpos, parent);
	return true;
}

void cantordb::add_member(string set_name, Set* element) {
	if(emergency_shut_off) { return; }
	set_index[element->set_name] = element;
	Set* parent = set_index[set_name];
	auto pos = std::lower_bound(parent->has_element.begin(), parent->has_element.end(), element);
	if(pos != parent->has_element.end() && *pos == element) return;
	parent->has_element.insert(pos, element);
	auto mpos = std::lower_bound(element->member_of.begin(), element->member_of.end(), parent);
	element->member_of.insert(mpos, parent);
}

bool cantordb::remove_member(string set_name, string element_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(set_index.find(element_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + element_name + "\" not found.";
		return false;
	}
	

	Set* parent = set_index[set_name];
	Set* element = set_index[element_name];

	auto& has_elem_vec = parent->has_element;
	auto it = std::lower_bound(has_elem_vec.begin(), has_elem_vec.end(), element);
	if(it == has_elem_vec.end() || *it != element) {
		EC = ER_ELEM_NOT_MEM;
		error_message = "Error: Element not a member of " + element_name;
		return false;
	}
	has_elem_vec.erase(it);

	auto& member_of_vec = element->member_of;
	auto mit = std::lower_bound(member_of_vec.begin(), member_of_vec.end(), parent);
	member_of_vec.erase(mit);

	return true;
}

Set* cantordb::set_union(string set_a_name, string set_b_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(set_index.find(set_a_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_a_name + "\" not found.";
		return nullptr;
	}
	if(set_index.find(set_b_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_b_name + "\" not found.";
		return nullptr;
	}

	Set* set_a = set_index[set_a_name];
	Set* set_b = set_index[set_b_name];
	return set_union(set_a, set_b);
}

Set* cantordb::set_union(Set* set_a, Set* set_b) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(!set_a || !set_b) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Null set pointer.";
		return nullptr;
	}
	Set* result = new (nothrow) Set();
	if(!result) { emergency_shut_off = true; error_message = "Error: Out of memory."; return nullptr; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete result; return nullptr; }
	result->set_name = set_a->set_name + " ∪ " + set_b->set_name;

	auto& a = set_a->has_element;
	auto& b = set_b->has_element;
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));
	int i = 0, j = 0;
	while(i < (int)a.size() && j < (int)b.size()) {
		if(a[i] < b[j]) {
			result->has_element.push_back(a[i++]);
		} else if(a[i] > b[j]) {
			result->has_element.push_back(b[j++]);
		} else {
			result->has_element.push_back(a[i++]);
			j++;
		}
	}
	while(i < (int)a.size()) result->has_element.push_back(a[i++]);
	while(j < (int)b.size()) result->has_element.push_back(b[j++]);

	add_member(CACHE_SET, result);
	return result;
}

Set* cantordb::set_intersection(string set_a_name, string set_b_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(set_index.find(set_a_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_a_name + "\" not found.";
		return nullptr;
	}
	if(set_index.find(set_b_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_b_name + "\" not found.";
		return nullptr;
	}

	Set* set_a = set_index[set_a_name];
	Set* set_b = set_index[set_b_name];
	return set_intersection(set_a, set_b);
}

Set* cantordb::set_intersection(Set* set_a, Set* set_b) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(!set_a || !set_b) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Null set pointer.";
		return nullptr;
	}
	Set* result = new (nothrow) Set();
	if(!result) { emergency_shut_off = true; error_message = "Error: Out of memory."; return nullptr; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete result; return nullptr; }
	result->set_name = set_a->set_name + " ∩ " + set_b->set_name;

	auto& a = set_a->has_element;
	auto& b = set_b->has_element;
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));
	int i = 0, j = 0;
	while(i < (int)a.size() && j < (int)b.size()) {
		if(a[i] < b[j]) {
			i++;
		} else if(a[i] > b[j]) {
			j++;
		} else {
			result->has_element.push_back(a[i++]);
			j++;
		}
	}

	add_member(CACHE_SET, result);
	return result;
}

Set* cantordb::set_difference(string set_a_name, string set_b_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(set_index.find(set_a_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_a_name + "\" not found.";
		return nullptr;
	}
	if(set_index.find(set_b_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_b_name + "\" not found.";
		return nullptr;
	}

	Set* set_a = set_index[set_a_name];
	Set* set_b = set_index[set_b_name];
	return set_difference(set_a, set_b);
}

Set* cantordb::set_difference(Set* set_a, Set* set_b) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(!set_a || !set_b) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Null set pointer.";
		return nullptr;
	}
	Set* result = new (nothrow) Set();
	if(!result) { emergency_shut_off = true; error_message = "Error: Out of memory."; return nullptr; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete result; return nullptr; }
	result->set_name = set_a->set_name + " - " + set_b->set_name;

	auto& a = set_a->has_element;
	auto& b = set_b->has_element;
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));
	int i = 0, j = 0;
	while(i < (int)a.size() && j < (int)b.size()) {
		if(a[i] < b[j]) {
			result->has_element.push_back(a[i++]);
		} else if(a[i] > b[j]) {
			j++;
		} else {
			i++;
			j++;
		}
	}
	while(i < (int)a.size()) result->has_element.push_back(a[i++]);

	add_member(CACHE_SET, result);
	return result;
}

bool cantordb::is_disjoint(string set_a_name, string set_b_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_a_name) == set_index.end() || set_index.find(set_b_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set not found.";
		return false;
	}
	return is_disjoint(set_index[set_a_name], set_index[set_b_name]);
}

bool cantordb::is_disjoint(Set* set_a, Set* set_b) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(!set_a || !set_b) return false;
	auto& a = set_a->has_element;
	auto& b = set_b->has_element;
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));
	int i = 0, j = 0;
	while(i < (int)a.size() && j < (int)b.size()) {
		if(a[i] < b[j]) {
			i++;
		} else if(a[i] > b[j]) {
			j++;
		} else {
			return false;
		}
	}
	return true;
}

Set* cantordb::symmetric_difference(string set_a_name, string set_b_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(set_index.find(set_a_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_a_name + "\" not found.";
		return nullptr;
	}
	if(set_index.find(set_b_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_b_name + "\" not found.";
		return nullptr;
	}
	return symmetric_difference(set_index[set_a_name], set_index[set_b_name]);
}

Set* cantordb::symmetric_difference(Set* set_a, Set* set_b) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(!set_a || !set_b) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Null set pointer.";
		return nullptr;
	}
	Set* result = new (nothrow) Set();
	if(!result) { emergency_shut_off = true; error_message = "Error: Out of memory."; return nullptr; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete result; return nullptr; }
	result->set_name = set_a->set_name + " ⊕ " + set_b->set_name;

	auto& a = set_a->has_element;
	auto& b = set_b->has_element;
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));
	int i = 0, j = 0;
	while(i < (int)a.size() && j < (int)b.size()) {
		if(a[i] < b[j]) {
			result->has_element.push_back(a[i++]);
		} else if(a[i] > b[j]) {
			result->has_element.push_back(b[j++]);
		} else {
			i++;
			j++;
		}
	}
	while(i < (int)a.size()) result->has_element.push_back(a[i++]);
	while(j < (int)b.size()) result->has_element.push_back(b[j++]);

	add_member(CACHE_SET, result);
	return result;
}

Set* cantordb::set_complement(string set_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return nullptr;
	}
	Set* complement = set_difference(UNIVERSAL_SET, set_name);
	if(!complement) return nullptr;
	string old_name = complement->set_name;
	string new_name = set_name + "'";
	complement->set_name = new_name;
	set_index[new_name] = complement;
	set_index.erase(old_name);
	return complement;
}
bool cantordb::is_subset(string set_a_name, string set_b_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_a_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_a_name + "\" not found.";
		return false;
	}
	if(set_index.find(set_b_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_b_name + "\" not found.";
		return false;
	}

	return is_subset(set_index[set_a_name], set_index[set_b_name]);
}

bool cantordb::is_subset(Set* set_a, Set* set_b) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(!set_a || !set_b) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Null set pointer.";
		return false;
	}
	auto& sub = set_a->has_element;
	auto& sup = set_b->has_element;
	assert(std::is_sorted(sub.begin(), sub.end()));
	assert(std::is_sorted(sup.begin(), sup.end()));
	if(sub.empty()) return true;
	if(sup.empty()) return false;
	if(sub.size() > sup.size()) return false;

	int i = 0, j = 0;
	while(i < (int)sub.size() && j < (int)sup.size()) {
		if(sub[i] == sup[j]) {
			i++;
			j++;
		} else if(sub[i] > sup[j]) {
			j++;
		} else {
			return false;
		}
	}
	return i == (int)sub.size();
}

bool cantordb::is_superset(string set_a_name, string set_b_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	return is_subset(set_b_name, set_a_name);
}

bool cantordb::is_superset(Set* set_a, Set* set_b) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	return is_subset(set_b, set_a);
}

bool cantordb::is_equal(string set_a_name, string set_b_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_a_name) == set_index.end() || set_index.find(set_b_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set not found.";
		return false;
	}
	return is_equal(set_index[set_a_name], set_index[set_b_name]);
}

bool cantordb::is_equal(Set* set_a, Set* set_b) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(!set_a || !set_b) return false;
	auto& a = set_a->has_element;
	auto& b = set_b->has_element;
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));
	if(a.size() != b.size()) return false;
	for(int i = 0; i < (int)a.size(); i++) {
		if(a[i] != b[i]) return false;
	}
	return true;
}

bool cantordb::is_proper_subset(string set_a_name, string set_b_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_a_name) == set_index.end() || set_index.find(set_b_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set not found.";
		return false;
	}
	return is_proper_subset(set_index[set_a_name], set_index[set_b_name]);
}

bool cantordb::is_proper_subset(Set* set_a, Set* set_b) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(!set_a || !set_b) return false;
	if(set_a->has_element.size() >= set_b->has_element.size()) return false;
	return is_subset(set_a, set_b);
}
variant<bool, long, string, int, double> cantordb::get_property(string set_name, string key_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(set_index[set_name]->key_index.find(key_name) == set_index[set_name]->key_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Key \"" + key_name + "\" not found in set \"" + set_name + "\".";
		return false;
	}
	switch(set_index[set_name]->key_index[key_name]) {
		case 0: return set_index[set_name]->key_num[key_name];
		case 1: return set_index[set_name]->key_value[key_name];
		case 2: return set_index[set_name]->key_decimal[key_name];
		case 3: return set_index[set_name]->key_bool[key_name];
		case 4: return set_index[set_name]->key_long[key_name];
		default: return false;
	}
}

int cantordb::get_property_safe_int(string set_name, string key_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return -1; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return 0;
	}
	if(set_index[set_name]->key_index.find(key_name) == set_index[set_name]->key_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Key \"" + key_name + "\" not found in set \"" + set_name + "\".";
		return 0;
	}
	if(set_index[set_name]->key_index[key_name] != 0) {
		EC = ER_KEY_WRONG_TYPE;
		error_message = "Error: Key \"" + key_name + "\" is not int type.";
		return 0;
	}
	return set_index[set_name]->key_num[key_name];
}

string cantordb::get_property_safe_string(string set_name, string key_name) {
	if(emergency_shut_off) { return "Error: Database emergency shut off."; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return "";
	}
	if(set_index[set_name]->key_index.find(key_name) == set_index[set_name]->key_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Key \"" + key_name + "\" not found in set \"" + set_name + "\".";
		return "";
	}
	if(set_index[set_name]->key_index[key_name] != 1) {
		EC = ER_KEY_WRONG_TYPE;
		error_message = "Error: Key \"" + key_name + "\" is not string type.";
		return "";
	}
	return set_index[set_name]->key_value[key_name];
}

double cantordb::get_property_safe_double(string set_name, string key_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return 0.0; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return 0.0;
	}
	if(set_index[set_name]->key_index.find(key_name) == set_index[set_name]->key_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Key \"" + key_name + "\" not found in set \"" + set_name + "\".";
		return 0.0;
	}
	if(set_index[set_name]->key_index[key_name] != 2) {
		EC = ER_KEY_WRONG_TYPE;
		error_message = "Error: Key \"" + key_name + "\" is not double type.";
		return 0.0;
	}
	return set_index[set_name]->key_decimal[key_name];
}

bool cantordb::get_property_safe_bool(string set_name, string key_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(set_index[set_name]->key_index.find(key_name) == set_index[set_name]->key_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Key \"" + key_name + "\" not found in set \"" + set_name + "\".";
		return false;
	}
	if(set_index[set_name]->key_index[key_name] != 3) {
		EC = ER_KEY_WRONG_TYPE;
		error_message = "Error: Key \"" + key_name + "\" is not bool type.";
		return false;
	}
	return set_index[set_name]->key_bool[key_name];
}

long cantordb::get_property_safe_long(string set_name, string key_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return 0L; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return 0;
	}
	if(set_index[set_name]->key_index.find(key_name) == set_index[set_name]->key_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Key \"" + key_name + "\" not found in set \"" + set_name + "\".";
		return 0;
	}
	if(set_index[set_name]->key_index[key_name] != 4) {
		EC = ER_KEY_WRONG_TYPE;
		error_message = "Error: Key \"" + key_name + "\" is not long type.";
		return 0;
	}
	return set_index[set_name]->key_long[key_name];
}

bool cantordb::delete_property(string set_name, string key_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(set_index[set_name]->key_index.find(key_name) == set_index[set_name]->key_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Key \"" + key_name + "\" not found in set \"" + set_name + "\".";
		return false;
	}
	Set* target = set_index[set_name];
	int type_tag = target->key_index[key_name];

	// Remove from the property-type index
	auto remove_from_index = [&](unordered_map<string, vector<Set*>>& idx) {
		auto it = idx.find(key_name);
		if(it != idx.end()) {
			auto& vec = it->second;
			vec.erase(std::remove(vec.begin(), vec.end(), target), vec.end());
			if(vec.empty()) idx.erase(it);
		}
	};

	switch(type_tag) {
		case 0: target->key_num.erase(key_name); remove_from_index(integer_property_index); break;
		case 1: target->key_value.erase(key_name); remove_from_index(string_property_index); break;
		case 2: target->key_decimal.erase(key_name); remove_from_index(double_property_index); break;
		case 3: target->key_bool.erase(key_name); remove_from_index(bool_property_index); break;
		case 4: target->key_long.erase(key_name); remove_from_index(long_property_index); break;
		default: return false;
	}
	target->key_index.erase(key_name);
	return true;
}
bool cantordb::update_property(string set_name, string key_name, int value) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(set_index[set_name]->key_index.find(key_name) == set_index[set_name]->key_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Key \"" + key_name + "\" not found in set \"" + set_name + "\".";
		return false;
	}
	if(set_index[set_name]->key_index[key_name] != 0) {
		EC = ER_KEY_WRONG_TYPE;
		error_message = "Error: Key \"" + key_name + "\" is not int type.";
		return false;
	}
	set_index[set_name]->key_num[key_name] = value;
	return true;
}

bool cantordb::update_property(string set_name, string key_name, string value) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(set_index[set_name]->key_index.find(key_name) == set_index[set_name]->key_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Key \"" + key_name + "\" not found in set \"" + set_name + "\".";
		return false;
	}
	if(set_index[set_name]->key_index[key_name] != 1) {
		EC = ER_KEY_WRONG_TYPE;
		error_message = "Error: Key \"" + key_name + "\" is not string type.";
		return false;
	}
	set_index[set_name]->key_value[key_name] = value;
	return true;
}

bool cantordb::update_property(string set_name, string key_name, double value) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(set_index[set_name]->key_index.find(key_name) == set_index[set_name]->key_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Key \"" + key_name + "\" not found in set \"" + set_name + "\".";
		return false;
	}
	if(set_index[set_name]->key_index[key_name] != 2) {
		EC = ER_KEY_WRONG_TYPE;
		error_message = "Error: Key \"" + key_name + "\" is not double type.";
		return false;
	}
	set_index[set_name]->key_decimal[key_name] = value;
	return true;
}

bool cantordb::update_property(string set_name, string key_name, bool value) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(set_index[set_name]->key_index.find(key_name) == set_index[set_name]->key_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Key \"" + key_name + "\" not found in set \"" + set_name + "\".";
		return false;
	}
	if(set_index[set_name]->key_index[key_name] != 3) {
		EC = ER_KEY_WRONG_TYPE;
		error_message = "Error: Key \"" + key_name + "\" is not bool type.";
		return false;
	}
	set_index[set_name]->key_bool[key_name] = value;
	return true;
}

bool cantordb::update_property(string set_name, string key_name, long value) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(set_index[set_name]->key_index.find(key_name) == set_index[set_name]->key_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: Key \"" + key_name + "\" not found in set \"" + set_name + "\".";
		return false;
	}
	if(set_index[set_name]->key_index[key_name] != 4) {
		EC = ER_KEY_WRONG_TYPE;
		error_message = "Error: Key \"" + key_name + "\" is not long type.";
		return false;
	}
	set_index[set_name]->key_long[key_name] = value;
	return true;
}

string cantordb::list_all_keys() {
	if(emergency_shut_off) { return "Error: Database emergency shut off."; }
	string result;
	for(auto& [key, type] : property_types) {
		string type_str;
		switch(type) {
			case INTEGER: type_str = "int"; break;
			case STRING:  type_str = "string"; break;
			case DOUBLE:  type_str = "double"; break;
			case BOOL:    type_str = "bool"; break;
			case LONG:    type_str = "long"; break;
		}
		if(!result.empty()) result += "\n";
		result += key + ": " + type_str;
	}
	return result;
}

string cantordb::list_property_keys(string set_name) {
	if(emergency_shut_off) { return "Error: Database emergency shut off."; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return "";
	}

	Set* s = set_index[set_name];
	string result;
	for (auto& [key, idx] : s->key_index) {
		string type;
		if (s->key_num.count(key))          type = "int";
		else if (s->key_value.count(key))   type = "string";
		else if (s->key_decimal.count(key)) type = "double";
		else if (s->key_bool.count(key))    type = "bool";
		else if (s->key_long.count(key))    type = "long";
		else                                type = "unknown";

		if (!result.empty()) result += "\n";
		result += key + ": " + type;
	}
	return result;
}
string cantordb::list_properties(string set_name) {
	if(emergency_shut_off) { return "Error: Database emergency shut off."; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return "";
	}

	Set* s = set_index[set_name];
	string result;
	for (auto& [key, idx] : s->key_index) {
		string type, value;
		if (s->key_num.count(key)) {
			type = "int";
			value = to_string(s->key_num[key]);
		} else if (s->key_value.count(key)) {
			type = "string";
			value = s->key_value[key];
		} else if (s->key_decimal.count(key)) {
			type = "double";
			value = to_string(s->key_decimal[key]);
		} else if (s->key_bool.count(key)) {
			type = "bool";
			value = s->key_bool[key] ? "true" : "false";
		} else if (s->key_long.count(key)) {
			type = "long";
			value = to_string(s->key_long[key]);
		} else {
			type = "unknown";
			value = "";
		}

		if (!result.empty()) result += "\n";
		result += key + ":" + value + ":" + type;
	}
	return result;
}
string cantordb::list_string_properties(string set_name) {
	if(emergency_shut_off) { return "Error: Database emergency shut off."; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return "";
	}

	Set* s = set_index[set_name];
	string result;
	for (auto& [key, val] : s->key_value) {
		if (!result.empty()) result += "\n";
		result += key + ":" + val;
	}
	return result;
}

string cantordb::list_int_properties(string set_name) {
	if(emergency_shut_off) { return "Error: Database emergency shut off."; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return "";
	}

	Set* s = set_index[set_name];
	string result;
	for (auto& [key, val] : s->key_num) {
		if (!result.empty()) result += "\n";
		result += key + ":" + to_string(val);
	}
	return result;
}

string cantordb::list_double_properties(string set_name) {
	if(emergency_shut_off) { return "Error: Database emergency shut off."; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return "";
	}

	Set* s = set_index[set_name];
	string result;
	for (auto& [key, val] : s->key_decimal) {
		if (!result.empty()) result += "\n";
		result += key + ":" + to_string(val);
	}
	return result;
}

string cantordb::list_bool_properties(string set_name) {
	if(emergency_shut_off) { return "Error: Database emergency shut off."; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return "";
	}

	Set* s = set_index[set_name];
	string result;
	for (auto& [key, val] : s->key_bool) {
		if (!result.empty()) result += "\n";
		result += key + ":" + (val ? "true" : "false");
	}
	return result;
}

string cantordb::list_long_properties(string set_name) {
	if(emergency_shut_off) { return "Error: Database emergency shut off."; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return "";
	}

	Set* s = set_index[set_name];
	string result;
	for (auto& [key, val] : s->key_long) {
		if (!result.empty()) result += "\n";
		result += key + ":" + to_string(val);
	}
	return result;
}

string cantordb::list_elements(string set_name) {
	if(emergency_shut_off) { return "Error: Database emergency shut off."; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return "";
	}

	Set* s = set_index[set_name];
	string result;
	for (auto& element : s->has_element) {
		if (!result.empty()) result += "\n";
		result += element->set_name;
	}
	return result;
}

int cantordb::get_cardinality(string set_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return -1; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return -1;
	}
	return set_index[set_name]->has_element.size();
}

string cantordb::list_all_sets() {
	if(emergency_shut_off) { return "Error: Database emergency shut off."; }

	Set* s = set_index[UNIVERSAL_SET];
	string result;
	for (auto& element : s->has_element) {
		if (!result.empty()) result += "\n";
		result += element->set_name;
	}
	return result;
}

string cantordb::list_cached_sets() {
	if(emergency_shut_off) { return "Error: Database emergency shut off."; }

	Set* s = set_index[CACHE_SET];
	string result;
	for (auto& element : s->has_element) {
		if (!result.empty()) result += "\n";
		result += element->set_name;
	}
	return result;
}
bool cantordb::clear_cache() {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	Set* cache = set_index[CACHE_SET];
	// Copy the vector — delete_set modifies has_element during unlinking
	vector<Set*> to_delete = cache->has_element;
	for (auto& element : to_delete) {
		// delete_set unlinks from member_of/has_element, then deletes
		delete_set(element->set_name, false);
	}
	return true;
}

bool cantordb::rename_set(string set_name, string set_new_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return false;
	}
	if(set_index.find(set_new_name) != set_index.end()) {
		EC = ER_SET_EX;
		error_message = "Error: Set \"" + set_new_name + "\" already exists.";
		return false;
	}

	Set* s = set_index[set_name];
	s->set_name = set_new_name;
	set_index[set_new_name] = s;
	set_index.erase(set_name);
	return true;
}

string cantordb::list_sets(string set_name) {
	if(emergency_shut_off) { return "Error: Database emergency shut off."; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return "";
	}

	Set* s = set_index[set_name];
	string result;
	for (auto& element : s->member_of) {
		if(element->set_name == UNIVERSAL_SET) continue;
		if (!result.empty()) result += "\n";
		result += element->set_name;
	}
	return result;
}

bool cantordb::is_element(string set_a_name, string set_b_name) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return false; }
	if(set_index.find(set_a_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_a_name + "\" not found.";
		return false;
	}
	if(set_index.find(set_b_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_b_name + "\" not found.";
		return false;
	}
	Set* a = set_index[set_a_name];
	auto& elems = set_index[set_b_name]->has_element;
	return std::binary_search(elems.begin(), elems.end(), a);
}

// ---- Binary serialization helpers ----

static void write_string(ofstream& out, const string& s) {
	uint32_t len = s.size();
	out.write(reinterpret_cast<const char*>(&len), sizeof(len));
	out.write(s.data(), len);
}

static string read_string(ifstream& in) {
	uint32_t len;
	in.read(reinterpret_cast<char*>(&len), sizeof(len));
	if(!in.good() || len > 10000000) return "";
	string s(len, '\0');
	in.read(&s[0], len);
	return s;
}

// property type tags: 0=int, 1=string, 2=double, 3=bool, 4=long
static void write_properties(ofstream& out, const Set* s) {
	uint32_t count = s->key_index.size();
	out.write(reinterpret_cast<const char*>(&count), sizeof(count));

	for (auto& [key, val] : s->key_num) {
		write_string(out, key);
		uint8_t tag = 0; out.write(reinterpret_cast<const char*>(&tag), 1);
		out.write(reinterpret_cast<const char*>(&val), sizeof(val));
	}
	for (auto& [key, val] : s->key_value) {
		write_string(out, key);
		uint8_t tag = 1; out.write(reinterpret_cast<const char*>(&tag), 1);
		write_string(out, val);
	}
	for (auto& [key, val] : s->key_decimal) {
		write_string(out, key);
		uint8_t tag = 2; out.write(reinterpret_cast<const char*>(&tag), 1);
		out.write(reinterpret_cast<const char*>(&val), sizeof(val));
	}
	for (auto& [key, val] : s->key_bool) {
		write_string(out, key);
		uint8_t tag = 3; out.write(reinterpret_cast<const char*>(&tag), 1);
		out.write(reinterpret_cast<const char*>(&val), sizeof(val));
	}
	for (auto& [key, val] : s->key_long) {
		write_string(out, key);
		uint8_t tag = 4; out.write(reinterpret_cast<const char*>(&tag), 1);
		out.write(reinterpret_cast<const char*>(&val), sizeof(val));
	}
}

static void read_properties(ifstream& in, Set* s) {
	uint32_t count; in.read(reinterpret_cast<char*>(&count), sizeof(count));
	for (uint32_t i = 0; i < count; i++) {
		string key = read_string(in);
		uint8_t tag; in.read(reinterpret_cast<char*>(&tag), 1);
		s->key_index[key] = tag;
		if (tag == 0) {
			int val; in.read(reinterpret_cast<char*>(&val), sizeof(val));
			s->key_num[key] = val;
		} else if (tag == 1) {
			s->key_value[key] = read_string(in);
		} else if (tag == 2) {
			double val; in.read(reinterpret_cast<char*>(&val), sizeof(val));
			s->key_decimal[key] = val;
		} else if (tag == 3) {
			bool val; in.read(reinterpret_cast<char*>(&val), sizeof(val));
			s->key_bool[key] = val;
		} else if (tag == 4) {
			long val; in.read(reinterpret_cast<char*>(&val), sizeof(val));
			s->key_long[key] = val;
		}
	}
}

Set* cantordb::where_elements_greater_than(string set_name, string property, int value) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return nullptr;
	}
	if(integer_property_index.find(property) == integer_property_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: No integer property \"" + property + "\" in index.";
		return nullptr;
	}

	Set* parent = set_index[set_name];
	auto& a = parent->has_element;
	auto& b = integer_property_index[property];
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));

	Set* result = new (nothrow) Set();
	if(!result) { emergency_shut_off = true; error_message = "Error: Out of memory."; return nullptr; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete result; return nullptr; }
	result->set_name = set_name + "[" + property + " > " + to_string(value) + "]";

	int i = 0, j = 0;
	while(i < (int)a.size() && j < (int)b.size()) {
		if(a[i] < b[j]) { i++; }
		else if(a[i] > b[j]) { j++; }
		else { if(a[i]->key_num[property] > value) result->has_element.push_back(a[i]); i++; j++; }
	}

	add_member(CACHE_SET, result);
	return result;
}

Set* cantordb::where_elements_lesser_than(string set_name, string property, int value) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return nullptr;
	}
	if(integer_property_index.find(property) == integer_property_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: No integer property \"" + property + "\" in index.";
		return nullptr;
	}

	Set* parent = set_index[set_name];
	auto& a = parent->has_element;
	auto& b = integer_property_index[property];
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));

	Set* result = new (nothrow) Set();
	if(!result) { emergency_shut_off = true; error_message = "Error: Out of memory."; return nullptr; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete result; return nullptr; }
	result->set_name = set_name + "[" + property + " < " + to_string(value) + "]";

	int i = 0, j = 0;
	while(i < (int)a.size() && j < (int)b.size()) {
		if(a[i] < b[j]) { i++; }
		else if(a[i] > b[j]) { j++; }
		else { if(a[i]->key_num[property] < value) result->has_element.push_back(a[i]); i++; j++; }
	}

	add_member(CACHE_SET, result);
	return result;
}

Set* cantordb::where_elements_equal_than(string set_name, string property, int value) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return nullptr;
	}
	if(integer_property_index.find(property) == integer_property_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: No integer property \"" + property + "\" in index.";
		return nullptr;
	}

	Set* parent = set_index[set_name];
	auto& a = parent->has_element;
	auto& b = integer_property_index[property];
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));

	Set* result = new (nothrow) Set();
	if(!result) { emergency_shut_off = true; error_message = "Error: Out of memory."; return nullptr; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete result; return nullptr; }
	result->set_name = set_name + "[" + property + " == " + to_string(value) + "]";

	int i = 0, j = 0;
	while(i < (int)a.size() && j < (int)b.size()) {
		if(a[i] < b[j]) { i++; }
		else if(a[i] > b[j]) { j++; }
		else { if(a[i]->key_num[property] == value) result->has_element.push_back(a[i]); i++; j++; }
	}

	add_member(CACHE_SET, result);
	return result;
}

// ---- long overloads ----

Set* cantordb::where_elements_greater_than(string set_name, string property, long value) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return nullptr;
	}
	if(long_property_index.find(property) == long_property_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: No long property \"" + property + "\" in index.";
		return nullptr;
	}

	Set* parent = set_index[set_name];
	auto& a = parent->has_element;
	auto& b = long_property_index[property];
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));

	Set* result = new (nothrow) Set();
	if(!result) { emergency_shut_off = true; error_message = "Error: Out of memory."; return nullptr; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete result; return nullptr; }
	result->set_name = set_name + "[" + property + " > " + to_string(value) + "]";

	int i = 0, j = 0;
	while(i < (int)a.size() && j < (int)b.size()) {
		if(a[i] < b[j]) { i++; }
		else if(a[i] > b[j]) { j++; }
		else { if(a[i]->key_long[property] > value) result->has_element.push_back(a[i]); i++; j++; }
	}

	add_member(CACHE_SET, result);
	return result;
}

Set* cantordb::where_elements_lesser_than(string set_name, string property, long value) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return nullptr;
	}
	if(long_property_index.find(property) == long_property_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: No long property \"" + property + "\" in index.";
		return nullptr;
	}

	Set* parent = set_index[set_name];
	auto& a = parent->has_element;
	auto& b = long_property_index[property];
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));

	Set* result = new (nothrow) Set();
	if(!result) { emergency_shut_off = true; error_message = "Error: Out of memory."; return nullptr; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete result; return nullptr; }
	result->set_name = set_name + "[" + property + " < " + to_string(value) + "]";

	int i = 0, j = 0;
	while(i < (int)a.size() && j < (int)b.size()) {
		if(a[i] < b[j]) { i++; }
		else if(a[i] > b[j]) { j++; }
		else { if(a[i]->key_long[property] < value) result->has_element.push_back(a[i]); i++; j++; }
	}

	add_member(CACHE_SET, result);
	return result;
}

Set* cantordb::where_elements_equal_than(string set_name, string property, long value) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return nullptr;
	}
	if(long_property_index.find(property) == long_property_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: No long property \"" + property + "\" in index.";
		return nullptr;
	}

	Set* parent = set_index[set_name];
	auto& a = parent->has_element;
	auto& b = long_property_index[property];
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));

	Set* result = new (nothrow) Set();
	if(!result) { emergency_shut_off = true; error_message = "Error: Out of memory."; return nullptr; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete result; return nullptr; }
	result->set_name = set_name + "[" + property + " == " + to_string(value) + "]";

	int i = 0, j = 0;
	while(i < (int)a.size() && j < (int)b.size()) {
		if(a[i] < b[j]) { i++; }
		else if(a[i] > b[j]) { j++; }
		else { if(a[i]->key_long[property] == value) result->has_element.push_back(a[i]); i++; j++; }
	}

	add_member(CACHE_SET, result);
	return result;
}

// ---- double overloads ----

Set* cantordb::where_elements_greater_than(string set_name, string property, double value) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return nullptr;
	}
	if(double_property_index.find(property) == double_property_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: No double property \"" + property + "\" in index.";
		return nullptr;
	}

	Set* parent = set_index[set_name];
	auto& a = parent->has_element;
	auto& b = double_property_index[property];
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));

	Set* result = new (nothrow) Set();
	if(!result) { emergency_shut_off = true; error_message = "Error: Out of memory."; return nullptr; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete result; return nullptr; }
	result->set_name = set_name + "[" + property + " > " + to_string(value) + "]";

	int i = 0, j = 0;
	while(i < (int)a.size() && j < (int)b.size()) {
		if(a[i] < b[j]) { i++; }
		else if(a[i] > b[j]) { j++; }
		else { if(a[i]->key_decimal[property] > value) result->has_element.push_back(a[i]); i++; j++; }
	}

	add_member(CACHE_SET, result);
	return result;
}

Set* cantordb::where_elements_lesser_than(string set_name, string property, double value) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return nullptr;
	}
	if(double_property_index.find(property) == double_property_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: No double property \"" + property + "\" in index.";
		return nullptr;
	}

	Set* parent = set_index[set_name];
	auto& a = parent->has_element;
	auto& b = double_property_index[property];
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));

	Set* result = new (nothrow) Set();
	if(!result) { emergency_shut_off = true; error_message = "Error: Out of memory."; return nullptr; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete result; return nullptr; }
	result->set_name = set_name + "[" + property + " < " + to_string(value) + "]";

	int i = 0, j = 0;
	while(i < (int)a.size() && j < (int)b.size()) {
		if(a[i] < b[j]) { i++; }
		else if(a[i] > b[j]) { j++; }
		else { if(a[i]->key_decimal[property] < value) result->has_element.push_back(a[i]); i++; j++; }
	}

	add_member(CACHE_SET, result);
	return result;
}

Set* cantordb::where_elements_equal_than(string set_name, string property, double value) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return nullptr;
	}
	if(double_property_index.find(property) == double_property_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: No double property \"" + property + "\" in index.";
		return nullptr;
	}

	Set* parent = set_index[set_name];
	auto& a = parent->has_element;
	auto& b = double_property_index[property];
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));

	Set* result = new (nothrow) Set();
	if(!result) { emergency_shut_off = true; error_message = "Error: Out of memory."; return nullptr; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete result; return nullptr; }
	result->set_name = set_name + "[" + property + " == " + to_string(value) + "]";

	int i = 0, j = 0;
	while(i < (int)a.size() && j < (int)b.size()) {
		if(a[i] < b[j]) { i++; }
		else if(a[i] > b[j]) { j++; }
		else { if(a[i]->key_decimal[property] == value) result->has_element.push_back(a[i]); i++; j++; }
	}

	add_member(CACHE_SET, result);
	return result;
}

Set* cantordb::where_elements_equal_than(string set_name, string property, string value) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return nullptr;
	}
	if(string_property_index.find(property) == string_property_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: No string property \"" + property + "\" in index.";
		return nullptr;
	}

	Set* parent = set_index[set_name];
	auto& a = parent->has_element;
	auto& b = string_property_index[property];
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));

	Set* result = new (nothrow) Set();
	if(!result) { emergency_shut_off = true; error_message = "Error: Out of memory."; return nullptr; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete result; return nullptr; }
	result->set_name = set_name + "[" + property + " == " + value + "]";

	int i = 0, j = 0;
	while(i < (int)a.size() && j < (int)b.size()) {
		if(a[i] < b[j]) { i++; }
		else if(a[i] > b[j]) { j++; }
		else { if(a[i]->key_value[property] == value) result->has_element.push_back(a[i]); i++; j++; }
	}

	add_member(CACHE_SET, result);
	return result;
}

Set* cantordb::where_elements_equal_than(string set_name, string property, bool value) {
	if(emergency_shut_off) { error_message = "Error: Database emergency shut off."; return nullptr; }
	if(set_index.find(set_name) == set_index.end()) {
		EC = ER_SET_NOT_FOUND;
		error_message = "Error: Set \"" + set_name + "\" not found.";
		return nullptr;
	}
	if(bool_property_index.find(property) == bool_property_index.end()) {
		EC = ER_KEY_NOT_FOUND;
		error_message = "Error: No bool property \"" + property + "\" in index.";
		return nullptr;
	}

	Set* parent = set_index[set_name];
	auto& a = parent->has_element;
	auto& b = bool_property_index[property];
	assert(std::is_sorted(a.begin(), a.end()));
	assert(std::is_sorted(b.begin(), b.end()));

	Set* result = new (nothrow) Set();
	if(!result) { emergency_shut_off = true; error_message = "Error: Out of memory."; return nullptr; }
	memory_used += SET_BASE_COST;
	if(memory_limit > 0 && memory_used > memory_limit) { emergency_shut_off = true; error_message = "Error: Memory limit exceeded."; delete result; return nullptr; }
	result->set_name = set_name + "[" + property + " == " + (value ? "true" : "false") + "]";

	int i = 0, j = 0;
	while(i < (int)a.size() && j < (int)b.size()) {
		if(a[i] < b[j]) { i++; }
		else if(a[i] > b[j]) { j++; }
		else { if(a[i]->key_bool[property] == value) result->has_element.push_back(a[i]); i++; j++; }
	}

	add_member(CACHE_SET, result);
	return result;
}

// ---- Save (skips CACHE_SET and its members) ----

bool save_cantordb(const cantordb& db, const string& path) {
	string tmp_path = path + ".tmp";
	ofstream out(tmp_path, ios::binary);
	if (!out) return false;

	write_string(out, db.name);

	// Collect sets to save (skip UNIVERSAL_SET and CACHE_SET)
	vector<pair<string, Set*>> sets_to_save;
	for (auto& [name, s] : db.set_index) {
		if (name != UNIVERSAL_SET && name != CACHE_SET) {
			sets_to_save.push_back({name, s});
		}
	}

	// sets
	uint32_t set_count = sets_to_save.size();
	out.write(reinterpret_cast<const char*>(&set_count), sizeof(set_count));
	for (auto& [name, s] : sets_to_save) {
		write_string(out, name);
		write_properties(out, s);
	}

	// membership relationships (parent -> child, skip UNIVERSAL_SET and CACHE_SET)
	vector<pair<string, string>> memberships;
	for (auto& [name, s] : sets_to_save) {
		for (Set* child : s->has_element) {
			if (child->set_name != UNIVERSAL_SET && child->set_name != CACHE_SET) {
				memberships.push_back({name, child->set_name});
			}
		}
	}

	uint32_t mem_count = memberships.size();
	out.write(reinterpret_cast<const char*>(&mem_count), sizeof(mem_count));
	for (auto& [parent, child] : memberships) {
		write_string(out, parent);
		write_string(out, child);
	}

	// property type registry
	uint32_t prop_count = db.property_types.size();
	out.write(reinterpret_cast<const char*>(&prop_count), sizeof(prop_count));
	for (auto& [name, type] : db.property_types) {
		write_string(out, name);
		uint8_t t = static_cast<uint8_t>(type);
		out.write(reinterpret_cast<const char*>(&t), sizeof(t));
	}

	if (!out.good()) { out.close(); remove(tmp_path.c_str()); return false; }
	out.close();
	remove(path.c_str());
	rename(tmp_path.c_str(), path.c_str());
	return true;
}

// ---- Load ----

cantordb* load_cantordb(const string& path) {
	ifstream in(path, ios::binary);
	if (!in.is_open()) return nullptr;

	string db_name = read_string(in);
	cantordb* db = new cantordb(db_name);

	// sets
	uint32_t set_count; in.read(reinterpret_cast<char*>(&set_count), sizeof(set_count));
	for (uint32_t i = 0; i < set_count; i++) {
		string set_name = read_string(in);
		db->create_set(set_name);
		read_properties(in, db->set_index[set_name]);
	}

	// memberships
	uint32_t mem_count; in.read(reinterpret_cast<char*>(&mem_count), sizeof(mem_count));
	for (uint32_t i = 0; i < mem_count; i++) {
		string parent = read_string(in);
		string child = read_string(in);
		db->add_member(parent, child);
	}

	if (!in.good()) { delete db; return nullptr; }

	// property type registry
	uint32_t prop_count = 0;
	if (in.read(reinterpret_cast<char*>(&prop_count), sizeof(prop_count))) {
		for (uint32_t i = 0; i < prop_count; i++) {
			string prop_name = read_string(in);
			uint8_t t; in.read(reinterpret_cast<char*>(&t), sizeof(t));
			db->property_types[prop_name] = static_cast<TYPE>(t);
		}
	}

	// Build property indexes
	for (auto& [name, s] : db->set_index) {
		if (name == UNIVERSAL_SET || name == CACHE_SET) continue;
		for (auto& [key, val] : s->key_num) {
			auto& vec = db->integer_property_index[key];
			auto pos = std::lower_bound(vec.begin(), vec.end(), s);
			vec.insert(pos, s);
		}
		for (auto& [key, val] : s->key_value) {
			auto& vec = db->string_property_index[key];
			auto pos = std::lower_bound(vec.begin(), vec.end(), s);
			vec.insert(pos, s);
		}
		for (auto& [key, val] : s->key_decimal) {
			auto& vec = db->double_property_index[key];
			auto pos = std::lower_bound(vec.begin(), vec.end(), s);
			vec.insert(pos, s);
		}
		for (auto& [key, val] : s->key_bool) {
			auto& vec = db->bool_property_index[key];
			auto pos = std::lower_bound(vec.begin(), vec.end(), s);
			vec.insert(pos, s);
		}
		for (auto& [key, val] : s->key_long) {
			auto& vec = db->long_property_index[key];
			auto pos = std::lower_bound(vec.begin(), vec.end(), s);
			vec.insert(pos, s);
		}
	}

	return db;
}
