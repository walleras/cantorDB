#include "cantordb.h"
#include "cdb_c.h"

// Every forwarded call clears the C3 error first and copies it into error_message if the call
// raised one. error_message is otherwise left alone (parse_query resets it per query), which is
// how the C++ implementation behaved.
struct cantordb::Call {
	cantordb& d;
	explicit Call(cantordb& db) : d(db) { cdb_clear_error(d.h); }
	~Call() {
		if(cdb_has_error(d.h)) {
			char* m = cdb_error_message(d.h);
			d.error_message = m;
			cdb_free_cstr(d.h, m);
			d.EC = (ERROR_CODE)cdb_error_code(d.h);
		}
	}
};

static inline cdb_set* raw(Set* s) { return reinterpret_cast<cdb_set*>(s); }
static inline Set* wrap(cdb_set* s) { return reinterpret_cast<Set*>(s); }

string cantordb::take(char* p) {
	string s = p ? p : "";
	cdb_free_cstr(h, p);
	return s;
}

cantordb::cantordb() : EC(ER_SET_EX), h(nullptr) {}

cantordb::cantordb(string db_name) : name(db_name), EC(ER_SET_EX), h(cdb_new(db_name.c_str(), 0)) {}

cantordb::cantordb(string db_name, size_t memory_limit_bytes) : name(db_name), EC(ER_SET_EX), h(cdb_new(db_name.c_str(), memory_limit_bytes)) {}

cantordb::~cantordb() { if(h) cdb_delete(h); }

bool cantordb::load(const string& path) {
	cdb_db* loaded = cdb_load(path.c_str());
	if(!loaded) return false;
	if(h) cdb_delete(h);
	h = loaded;
	name = take(cdb_name(h));
	error_message = "";
	return true;
}

bool cantordb::save(const string& path) const {
	return cdb_save(h, path.c_str());
}

bool save_cantordb(const cantordb& db, const string& path) { return db.save(path); }

cantordb* load_cantordb(const string& path) {
	cantordb* db = new cantordb();
	if(!db->load(path)) { delete db; return nullptr; }
	return db;
}

// ---- sets ----

bool cantordb::create_set(string set_name) { Call c(*this); return cdb_create_set(h, set_name.c_str(), false); }
bool cantordb::create_set(string set_name, bool gen_set) { Call c(*this); return cdb_create_set(h, set_name.c_str(), gen_set); }
bool cantordb::delete_set(string set_name, bool safe) { Call c(*this); return cdb_delete_set(h, set_name.c_str(), safe); }
bool cantordb::restore_set(string set_name) { Call c(*this); return cdb_restore_set(h, set_name.c_str()); }
bool cantordb::rename_set(string set_name, string set_new_name) { Call c(*this); return cdb_rename_set(h, set_name.c_str(), set_new_name.c_str()); }
bool cantordb::add_member(string set_name, string element_name) { Call c(*this); return cdb_add_member(h, set_name.c_str(), element_name.c_str()); }
bool cantordb::remove_member(string set_name, string element_name) { Call c(*this); return cdb_remove_member(h, set_name.c_str(), element_name.c_str()); }
bool cantordb::clear_cache() { Call c(*this); return cdb_clear_cache(h); }

Set* cantordb::combine(const string& a, const string& b, int op) { Call c(*this); return wrap(cdb_combine_named(h, a.c_str(), b.c_str(), op)); }

Set* cantordb::set_union(string a, string b) { return combine(a, b, 0); }
Set* cantordb::set_intersection(string a, string b) { return combine(a, b, 1); }
Set* cantordb::set_difference(string a, string b) { return combine(a, b, 2); }
Set* cantordb::symmetric_difference(string a, string b) { return combine(a, b, 3); }
Set* cantordb::set_union(Set* a, Set* b) { Call c(*this); return wrap(cdb_combine(h, raw(a), raw(b), 0)); }
Set* cantordb::set_intersection(Set* a, Set* b) { Call c(*this); return wrap(cdb_combine(h, raw(a), raw(b), 1)); }
Set* cantordb::set_difference(Set* a, Set* b) { Call c(*this); return wrap(cdb_combine(h, raw(a), raw(b), 2)); }
Set* cantordb::symmetric_difference(Set* a, Set* b) { Call c(*this); return wrap(cdb_combine(h, raw(a), raw(b), 3)); }
Set* cantordb::set_complement(string set_name) { Call c(*this); return wrap(cdb_complement(h, set_name.c_str())); }

bool cantordb::relation(const string& a, const string& b, int kind) { Call c(*this); return cdb_relation(h, a.c_str(), b.c_str(), kind); }

bool cantordb::is_element(string a, string b) { return relation(a, b, 0); }
bool cantordb::is_disjoint(string a, string b) { return relation(a, b, 1); }
bool cantordb::is_subset(string a, string b) { return relation(a, b, 2); }
bool cantordb::is_superset(string a, string b) { return relation(a, b, 3); }
bool cantordb::is_equal(string a, string b) { return relation(a, b, 4); }
bool cantordb::is_proper_subset(string a, string b) { return relation(a, b, 5); }

int cantordb::get_cardinality(string set_name) { Call c(*this); return cdb_cardinality(h, set_name.c_str()); }

string cantordb::list_elements(string set_name) { Call c(*this); return take(cdb_list_elements(h, set_name.c_str())); }
string cantordb::list_sets(string set_name) { Call c(*this); return take(cdb_list_sets(h, set_name.c_str())); }
string cantordb::list_all_sets() { Call c(*this); return take(cdb_list_all_sets(h)); }
string cantordb::list_all_keys() { Call c(*this); return take(cdb_list_all_keys(h)); }
string cantordb::list_property_keys(string set_name) { Call c(*this); return take(cdb_list_property_keys(h, set_name.c_str())); }
string cantordb::list_properties(string set_name) { Call c(*this); return take(cdb_list_properties(h, set_name.c_str())); }

Set* cantordb::find_set(const string& set_name) { return wrap(cdb_find_set(h, set_name.c_str())); }
string cantordb::set_name_of(Set* s) { return take(cdb_set_name(h, raw(s))); }
int cantordb::key_type(const string& set_name, const string& key) { return cdb_key_type(h, set_name.c_str(), key.c_str()); }
int cantordb::property_type(const string& property_name) { return cdb_property_type(h, property_name.c_str()); }

// ---- properties ----

bool cantordb::create_property(string property_name, string type) { Call c(*this); return cdb_create_property(h, property_name.c_str(), type.c_str()); }
bool cantordb::delete_property(string set_name, string key_name) { Call c(*this); return cdb_delete_property(h, set_name.c_str(), key_name.c_str()); }
bool cantordb::add_property(string property_name, string set_name) { Call c(*this); return cdb_add_property_default(h, property_name.c_str(), set_name.c_str()); }

bool cantordb::add_value(const string& key, int type, int64_t l, double d, const char* s, const string& set_name) {
	Call c(*this);
	return cdb_add_property(h, key.c_str(), type, l, d, s, set_name.c_str());
}
bool cantordb::update_value(const string& set_name, const string& key, int type, int64_t l, double d, const char* s) {
	Call c(*this);
	return cdb_update_property(h, set_name.c_str(), key.c_str(), type, l, d, s);
}

bool cantordb::add_property(string key, int value, string set_name) { return add_value(key, INTEGER, value, 0, "", set_name); }
bool cantordb::add_property(string key, string value, string set_name) { return add_value(key, STRING, 0, 0, value.c_str(), set_name); }
bool cantordb::add_property(string key, double value, string set_name) { return add_value(key, DOUBLE, 0, value, "", set_name); }
bool cantordb::add_property(string key, bool value, string set_name) { return add_value(key, BOOL, value ? 1 : 0, 0, "", set_name); }
bool cantordb::add_property(string key, int64_t value, string set_name) { return add_value(key, LONG, value, 0, "", set_name); }

bool cantordb::update_property(string set_name, string key_name, int value) { return update_value(set_name, key_name, INTEGER, value, 0, ""); }
bool cantordb::update_property(string set_name, string key_name, string value) { return update_value(set_name, key_name, STRING, 0, 0, value.c_str()); }
bool cantordb::update_property(string set_name, string key_name, double value) { return update_value(set_name, key_name, DOUBLE, 0, value, ""); }
bool cantordb::update_property(string set_name, string key_name, bool value) { return update_value(set_name, key_name, BOOL, value ? 1 : 0, 0, ""); }
bool cantordb::update_property(string set_name, string key_name, int64_t value) { return update_value(set_name, key_name, LONG, value, 0, ""); }

variant<bool, int64_t, string, int, double> cantordb::get_property(string set_name, string key_name) {
	Call c(*this);
	int type = 0; int64_t l = 0; double d = 0; char* s = nullptr;
	cdb_get_property(h, set_name.c_str(), key_name.c_str(), &type, &l, &d, &s);
	string str = take(s);
	switch(type) {
		case INTEGER: return (int)l;
		case STRING: return str;
		case DOUBLE: return d;
		case LONG: return (int64_t)l;
		default: return l != 0;
	}
}

Set* cantordb::where(const string& set_name, const string& property, int op, int type, int64_t l, double d, const char* s) {
	Call c(*this);
	return wrap(cdb_where(h, set_name.c_str(), property.c_str(), op, type, l, d, s));
}

Set* cantordb::where_elements_greater_than(string s, string p, int v) { return where(s, p, 0, INTEGER, v, 0, ""); }
Set* cantordb::where_elements_greater_than(string s, string p, int64_t v) { return where(s, p, 0, LONG, v, 0, ""); }
Set* cantordb::where_elements_greater_than(string s, string p, double v) { return where(s, p, 0, DOUBLE, 0, v, ""); }
Set* cantordb::where_elements_lesser_than(string s, string p, int v) { return where(s, p, 1, INTEGER, v, 0, ""); }
Set* cantordb::where_elements_lesser_than(string s, string p, int64_t v) { return where(s, p, 1, LONG, v, 0, ""); }
Set* cantordb::where_elements_lesser_than(string s, string p, double v) { return where(s, p, 1, DOUBLE, 0, v, ""); }
Set* cantordb::where_elements_equal_than(string s, string p, int v) { return where(s, p, 2, INTEGER, v, 0, ""); }
Set* cantordb::where_elements_equal_than(string s, string p, int64_t v) { return where(s, p, 2, LONG, v, 0, ""); }
Set* cantordb::where_elements_equal_than(string s, string p, double v) { return where(s, p, 2, DOUBLE, 0, v, ""); }
Set* cantordb::where_elements_equal_than(string s, string p, string v) { return where(s, p, 2, STRING, 0, 0, v.c_str()); }
Set* cantordb::where_elements_equal_than(string s, string p, bool v) { return where(s, p, 2, BOOL, v ? 1 : 0, 0, ""); }
