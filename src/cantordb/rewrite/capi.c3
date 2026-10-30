module cantordb;

import std::core::mem::alloc;

// C ABI over CantorDB, for the C++ shell/parser (see ../c3bridge).
//
//  - Strings in are NUL-terminated char*. Strings out are heap copies made with db.allocator;
//    release them with cdb_free_cstr.
//  - Sets are opaque Set* handles, valid until that set is deleted (cached results: until clear_cache).
//  - Values travel as (type tag, long, double, char*). Tags are the PropType ordinals:
//    0=int 1=string 2=double 3=bool 4=long.
//  - Errors: cdb_clear_error before a call, cdb_has_error / cdb_error_message after it.

fn ZString copy_out(CantorDB* db, String s)
{
	char[] buf = alloc::new_array(db.allocator, char, s.len + 1);
	if (s.len > 0) mem::copy(buf.ptr, s.ptr, s.len);
	buf[s.len] = 0;
	return (ZString)buf.ptr;
}

// Converts a String the database handed out (list_*) and releases the original.
fn ZString take_out(CantorDB* db, String s)
{
	ZString out = copy_out(db, s);
	db.free_string(s);
	return out;
}

fn PropertyValue make_pv(int type, long l, double d, ZString s)
{
	switch (type)
	{
		case 0: return pv_int((int)l);
		case 1: return pv_string(s.str_view());
		case 2: return pv_double(d);
		case 3: return pv_bool(l != 0);
		case 4: return pv_long(l);
		default: return pv_date(l);
	}
}

// ---- lifecycle ----

fn CantorDB* capi_new(ZString name, usz memory_limit) @export("cdb_new")
{
	return new_cantordb(name.str_view(), memory_limit);
}

fn void capi_delete(CantorDB* db) @export("cdb_delete") => delete_cantordb(db);

fn CantorDB* capi_load(ZString path) @export("cdb_load") => load_cantordb(path.str_view());

fn bool capi_save(CantorDB* db, ZString path) @export("cdb_save") => save_cantordb(db, path.str_view());

fn ZString capi_name(CantorDB* db) @export("cdb_name") => copy_out(db, db.name);

fn void capi_free_cstr(CantorDB* db, ZString p) @export("cdb_free_cstr")
{
	if (p) alloc::free(db.allocator, (char*)p);
}

// ---- errors ----

fn void capi_clear_error(CantorDB* db) @export("cdb_clear_error")
{
	db.error_buf.clear();
	db.error_message = "";
}

fn bool capi_has_error(CantorDB* db) @export("cdb_has_error") => db.error_message.len > 0;

fn ZString capi_error_message(CantorDB* db) @export("cdb_error_message") => copy_out(db, db.error_message);

fn int capi_error_code(CantorDB* db) @export("cdb_error_code") => (int)db.ec;

fn bool capi_shut_off(CantorDB* db) @export("cdb_shut_off") => db.emergency_shut_off;

// ---- sets ----

fn bool capi_create_set(CantorDB* db, ZString name, bool gen_set) @export("cdb_create_set") => db.create_set(name.str_view(), gen_set);

fn bool capi_delete_set(CantorDB* db, ZString name, bool safe) @export("cdb_delete_set") => db.delete_set(name.str_view(), safe);

fn bool capi_restore_set(CantorDB* db, ZString name) @export("cdb_restore_set") => db.restore_set(name.str_view());

fn bool capi_rename_set(CantorDB* db, ZString from, ZString to) @export("cdb_rename_set") => db.rename_set(from.str_view(), to.str_view());

fn bool capi_add_member(CantorDB* db, ZString set, ZString elem) @export("cdb_add_member") => db.add_member(set.str_view(), elem.str_view());

fn bool capi_remove_member(CantorDB* db, ZString set, ZString elem) @export("cdb_remove_member") => db.remove_member(set.str_view(), elem.str_view());

fn bool capi_clear_cache(CantorDB* db) @export("cdb_clear_cache") => db.clear_cache();

// No error side effects; null when there is no such set.
fn Set* capi_find_set(CantorDB* db, ZString name) @export("cdb_find_set") => db.find_set(name.str_view());

fn ZString capi_set_name(CantorDB* db, Set* s) @export("cdb_set_name") => copy_out(db, s.set_name);

fn int capi_cardinality(CantorDB* db, ZString name) @export("cdb_cardinality") => db.get_cardinality(name.str_view());

// op: 0=union 1=intersection 2=difference 3=symmetric difference
fn Set* capi_combine_named(CantorDB* db, ZString a, ZString b, int op) @export("cdb_combine_named")
{
	return db.combine_named(a.str_view(), b.str_view(), (SetOp)op);
}

fn Set* capi_combine(CantorDB* db, Set* a, Set* b, int op) @export("cdb_combine")
{
	return db.combine(a, b, (SetOp)op);
}

fn Set* capi_complement(CantorDB* db, ZString name) @export("cdb_complement") => db.set_complement(name.str_view());

// kind: 0=is_element(a of b) 1=is_disjoint 2=is_subset 3=is_superset 4=is_equal 5=is_proper_subset
fn bool capi_relation(CantorDB* db, ZString a, ZString b, int kind) @export("cdb_relation")
{
	String x = a.str_view();
	String y = b.str_view();
	switch (kind)
	{
		case 0: return db.is_element(x, y);
		case 1: return db.is_disjoint(x, y);
		case 2: return db.is_subset(x, y);
		case 3: return db.is_superset(x, y);
		case 4: return db.is_equal(x, y);
		default: return db.is_proper_subset(x, y);
	}
}

// ---- listings ----

fn ZString capi_list_elements(CantorDB* db, ZString set) @export("cdb_list_elements") => take_out(db, db.list_elements(set.str_view()));
fn ZString capi_list_sets(CantorDB* db, ZString set) @export("cdb_list_sets") => take_out(db, db.list_sets(set.str_view()));
fn ZString capi_list_all_sets(CantorDB* db) @export("cdb_list_all_sets") => take_out(db, db.list_all_sets());
fn ZString capi_list_all_keys(CantorDB* db) @export("cdb_list_all_keys") => take_out(db, db.list_all_keys());
fn ZString capi_list_property_keys(CantorDB* db, ZString set) @export("cdb_list_property_keys") => take_out(db, db.list_property_keys(set.str_view()));
fn ZString capi_list_properties(CantorDB* db, ZString set) @export("cdb_list_properties") => take_out(db, db.list_properties(set.str_view()));

// ---- properties ----

fn bool capi_create_property(CantorDB* db, ZString name, ZString type) @export("cdb_create_property")
{
	return db.create_property(name.str_view(), type.str_view());
}

fn bool capi_delete_property(CantorDB* db, ZString set, ZString key) @export("cdb_delete_property")
{
	return db.delete_property(set.str_view(), key.str_view());
}

// Property with the type's default value.
fn bool capi_add_property_default(CantorDB* db, ZString name, ZString set) @export("cdb_add_property_default")
{
	return db.add_property(name.str_view(), set.str_view());
}

fn bool capi_add_property(CantorDB* db, ZString key, int type, long l, double d, ZString s, ZString set) @export("cdb_add_property")
{
	return db.add_property_value(key.str_view(), make_pv(type, l, d, s), set.str_view());
}

fn bool capi_update_property(CantorDB* db, ZString set, ZString key, int type, long l, double d, ZString s) @export("cdb_update_property")
{
	return db.update_property(set.str_view(), key.str_view(), make_pv(type, l, d, s));
}

// Reads a property. Check cdb_has_error afterwards. *s is a heap copy (cdb_free_cstr) when type==1, else null.
fn void capi_get_property(CantorDB* db, ZString set, ZString key, int* type, long* l, double* d, ZString* s) @export("cdb_get_property")
{
	PropertyValue v = db.get_property(set.str_view(), key.str_view());
	*type = (int)v.type;
	*l = 0;
	*d = 0;
	*s = null;
	switch (v.type)
	{
		case INTEGER: *l = v.value_int;
		case STRING: *s = copy_out(db, v.value_string);
		case DOUBLE: *d = v.value_double;
		case BOOL: *l = v.value_bool ? 1 : 0;
		case LONG: *l = v.value_long;
		default: *l = v.value_date;
	}
}

// Registered type tag of a property name, or -1 if it was never created.
fn int capi_property_type(CantorDB* db, ZString name) @export("cdb_property_type")
{
	PropType? t = db.property_type(name.str_view());
	if (catch t) return -1;
	return (int)t;
}

// Type tag of a property held by a set, or -1 if the set or key is missing. No error side effects.
fn int capi_key_type(CantorDB* db, ZString set, ZString key) @export("cdb_key_type")
{
	Set* s = db.find_set(set.str_view());
	if (!s) return -1;
	PropertyValue*? pv = s.properties.get_ref(key.str_view());
	if (catch pv) return -1;
	return (int)pv.type;
}

// op: 0=greater 1=lesser 2=equal
fn Set* capi_where(CantorDB* db, ZString set, ZString property, int op, int type, long l, double d, ZString s) @export("cdb_where")
{
	return db.where_elements(set.str_view(), property.str_view(), (CmpOp)op, make_pv(type, l, d, s));
}
