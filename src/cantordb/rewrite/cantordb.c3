module cantordb;

import std::collections::map, std::collections::list;
import std::core::dstring;
import std::io::file;
import libc;
import std::core::mem::alloc;

// C3 port of cantordb.cpp / cantordb.h.
//
// Differences from the C++ that come from the language or the bitmap design:
//  - C3 has no overloading, so the per-type overloads (add_property, update_property,
//    where_elements_*) take a PropertyValue (build one with pv_int, pv_string, ...), and
//    the Set*-taking variants of the set operations end in _set.
//  - Sets are identified by uint ids; has_element / member_of are Roaring bitmaps of ids.
//  - Every String the database hands back from list_* / get_property_safe_string is
//    heap-allocated with db.allocator; release it with CantorDB.free_string. Strings from
//    get_property / PropertyValue are views into the database and die with the property.
//  - error_message is a view into an internal buffer; it is valid until the next error.

const String UNIVERSAL_SET = "__universal__";
const String CACHE_SET = "__cache__";
const String TRASH_SET = "__trash__";

enum ErrorCode : int { ER_SET_EX, ER_SET_NOT_FOUND, ER_KEY_EX, ER_ELEM_NOT_MEM, ER_ELEM_ALR_ADDED, ER_KEY_NOT_FOUND, ER_KEY_WRONG_TYPE }

// Saved files store the ordinal as the type tag: 0=int 1=string 2=double 3=bool 4=long (5=date)
enum PropType : int { INTEGER, STRING, DOUBLE, BOOL, LONG, DATE }

enum CmpOp { GREATER, LESSER, EQUAL }

enum SetOp { UNION, INTERSECTION, DIFFERENCE, SYMMETRIC }

struct PropertyValue
{
	PropType type;
	union
	{
		int value_int;
		String value_string;
		double value_double;
		bool value_bool;
		long value_long;
		long value_date;
	}
}

fn PropertyValue pv_int(int v) => { .type = INTEGER, .value_int = v };
fn PropertyValue pv_string(String v) => { .type = STRING, .value_string = v };
fn PropertyValue pv_double(double v) => { .type = DOUBLE, .value_double = v };
fn PropertyValue pv_bool(bool v) => { .type = BOOL, .value_bool = v };
fn PropertyValue pv_long(long v) => { .type = LONG, .value_long = v };
fn PropertyValue pv_date(long v) => { .type = DATE, .value_date = v };

fn PropertyValue pv_default(PropType t)
{
	switch (t)
	{
		case INTEGER: return pv_int(0);
		case STRING: return pv_string("");
		case DOUBLE: return pv_double(0.0);
		case BOOL: return pv_bool(false);
		case LONG: return pv_long(0);
		default: return pv_date(0);
	}
}

fn String type_name(PropType t)
{
	switch (t)
	{
		case INTEGER: return "int";
		case STRING: return "string";
		case DOUBLE: return "double";
		case BOOL: return "bool";
		case LONG: return "long";
		default: return "date";
	}
}

// the wording used in "No <x> property ... in index" errors
fn String type_word(PropType t) => t == INTEGER ? "integer" : type_name(t);

// temp-allocated; use immediately
fn String value_to_string(PropertyValue v)
{
	switch (v.type)
	{
		case INTEGER: return string::tformat("%d", v.value_int);
		case STRING: return v.value_string;
		case DOUBLE: return string::tformat("%f", v.value_double);
		case BOOL: return v.value_bool ? "true" : "false";
		case LONG: return string::tformat("%d", v.value_long);
		default: return string::tformat("%d", v.value_date);
	}
}

faultdef BAD_TYPE_TAG;

fn PropType? type_from_tag(int tag)
{
	switch (tag)
	{
		case 0: return INTEGER;
		case 1: return STRING;
		case 2: return DOUBLE;
		case 3: return BOOL;
		case 4: return LONG;
		case 5: return DATE;
		default: return BAD_TYPE_TAG~;
	}
}

fn bool str_less(String a, String b)
{
	usz n = a.len < b.len ? a.len : b.len;
	for (usz i = 0; i < n; i++)
	{
		if (a[i] != b[i]) return a[i] < b[i];
	}
	return a.len < b.len;
}

fn void sort_strings(String[] arr)
{
	for (usz i = 1; i < arr.len; i++)
	{
		String key = arr[i];
		usz j = i;
		while (j > 0 && str_less(key, arr[j - 1]))
		{
			arr[j] = arr[j - 1];
			j--;
		}
		arr[j] = key;
	}
}

macro bool cmp_op(CmpOp op, a, b)
{
	if (op == GREATER) return a > b;
	if (op == LESSER) return a < b;
	return a == b;
}

// ---- Registry: name <-> id ----

struct Registry
{
	Allocator heap;
	HashMap{String, uint} name_to_id;
	HashMap{uint, String} id_to_name;
	List{uint} free_ids;
	uint next_id;
}

fn void Registry.init(&self, Allocator heap)
{
	self.heap = heap;
	self.name_to_id.init(heap);
	self.id_to_name.init(heap);
	self.free_ids.init(heap);
	self.next_id = 0;
}

fn void Registry.free(&self)
{
	@pool()
	{
		foreach (id : self.id_to_name.tkeys())
		{
			if (try name = self.id_to_name.get(id)) alloc::free(self.heap, name.ptr);
		}
	};
	self.name_to_id.free();
	self.id_to_name.free();
	self.free_ids.free();
}

// Copies the name; the registry owns the copy until remove().
fn uint Registry.register(&self, String name)
{
	String owned = name.copy(self.heap);
	uint id;
	if (try reused = self.free_ids.pop())
	{
		id = reused;
	}
	else
	{
		id = self.next_id++;
	}
	self.name_to_id.set(owned, id);
	self.id_to_name.set(id, owned);
	return id;
}

fn uint? Registry.get_id(&self, String name) => self.name_to_id.get(name);

fn String? Registry.get_name(&self, uint id) => self.id_to_name.get(id);

fn bool Registry.has_name(&self, String name) => self.name_to_id.has_key(name);

// Moves an id to a new name; returns the registry-owned copy of the new name.
fn String Registry.rename(&self, uint id, String new_name)
{
	String owned = new_name.copy(self.heap);
	String? old = self.id_to_name.get(id);
	if (try old)
	{
		(void)self.name_to_id.remove(old);
		self.id_to_name.set(id, owned);
		self.name_to_id.set(owned, id);
		alloc::free(self.heap, old.ptr);
	}
	return owned;
}

// Soft delete: the name becomes free for reuse but the id (and its name string) stay reserved.
fn void Registry.unname(&self, uint id)
{
	if (try name = self.id_to_name.get(id))
	{
		if (try mapped = self.name_to_id.get(name))
		{
			if (mapped == id) (void)self.name_to_id.remove(name);
		}
	}
}

// Undo unname(). False if another set took the name in the meantime.
fn bool Registry.rename_back(&self, uint id)
{
	String? name = self.id_to_name.get(id);
	if (catch name) return false;
	if (self.name_to_id.has_key(name)) return false;
	self.name_to_id.set(name, id);
	return true;
}

// Hard delete: frees the name and recycles the id.
fn void Registry.remove(&self, uint id)
{
	if (try name = self.id_to_name.get(id))
	{
		self.unname(id);
		(void)self.id_to_name.remove(id);
		alloc::free(self.heap, name.ptr);
		self.free_ids.push(id);
	}
}

// ---- Set ----

struct Set
{
	uint id;
	String set_name;                                   // owned by the registry
	Bitmap* has_element;
	Bitmap* member_of;
	HashMap{String, PropertyValue} properties;         // keys owned by the property registry
}

fn Set*? build_set(uint id, String name, Allocator heap)
{
	Set* s = alloc::new_try(heap, Set)!;
	s.id = id;
	s.set_name = name;
	s.has_element = new_bitmap();
	s.member_of = new_bitmap();
	s.properties.init(heap);
	return s;
}

fn void Set.clear_properties(&self, Allocator heap)
{
	@pool()
	{
		foreach (key : self.properties.tkeys())
		{
			if (try pv = self.properties.get(key))
			{
				if (pv.type == STRING && pv.value_string.len > 0) alloc::free(heap, pv.value_string.ptr);
			}
		}
	};
	self.properties.clear();
}

fn void Set.destroy(&self, Allocator heap)
{
	self.clear_properties(heap);
	self.properties.free();
	roaring_bitmap_free(self.has_element);
	roaring_bitmap_free(self.member_of);
	alloc::free(heap, self);
}

// ---- CantorDB ----

const usz SET_BASE_COST = Set.sizeof + 64;

struct CantorDB
{
	String name;
	Allocator allocator;
	Registry registry;                       // set names
	Registry property_registry;              // property names
	HashMap{uint, Set*} sets;                    // every set object, by id (including soft-deleted ones)
	HashMap{uint, Set*} properties;              // property id -> set whose has_element = sets that have the property
	HashMap{uint, PropType} property_types;      // property id -> type
	ErrorCode ec;
	String error_message;
	DString error_buf;
	bool emergency_shut_off;
	usz memory_used;
	usz memory_limit;
}

fn CantorDB* CantorDB.init(&self, String name, usz memory_limit = 0, Allocator allocator = mem)
{
	*self = {};
	self.allocator = allocator;
	self.name = name.copy(allocator);
	self.memory_limit = memory_limit;
	self.registry.init(allocator);
	self.property_registry.init(allocator);
	self.sets.init(allocator);
	self.properties.init(allocator);
	self.property_types.init(allocator);
	self.error_buf = dstring::new_with_capacity(allocator, 128);
	self.create_set(UNIVERSAL_SET, true);
	self.create_set(CACHE_SET, true);
	self.create_set(TRASH_SET, true);
	return self;
}

// Frees every set, including soft-deleted ones and cached results.
fn void CantorDB.free(&self)
{
	@pool()
	{
		foreach (id : self.sets.tkeys())
		{
			if (try s = self.sets.get(id)) s.destroy(self.allocator);
		}
		foreach (id : self.properties.tkeys())
		{
			if (try s = self.properties.get(id)) s.destroy(self.allocator);
		}
	};
	self.sets.free();
	self.properties.free();
	self.property_types.free();
	self.registry.free();
	self.property_registry.free();
	self.error_buf.free();
	alloc::free(self.allocator, self.name.ptr);
}

fn CantorDB* new_cantordb(String name, usz memory_limit = 0, Allocator allocator = mem)
{
	CantorDB* db = alloc::new(allocator, CantorDB);
	return db.init(name, memory_limit, allocator);
}

fn void delete_cantordb(CantorDB* db)
{
	Allocator a = db.allocator;
	db.free();
	alloc::free(a, db);
}

// ---- errors and small helpers ----

fn void CantorDB.say(&self, String message)
{
	self.error_buf.clear();
	self.error_buf.append_string(message);
	self.error_message = self.error_buf.str_view();
}

fn void CantorDB.fail(&self, ErrorCode ec, String message)
{
	self.ec = ec;
	self.say(message);
}

fn bool CantorDB.shut_off(&self)
{
	if (!self.emergency_shut_off) return false;
	self.say("Error: Database emergency shut off.");
	return true;
}

fn String CantorDB.dup(&self, String s)
{
	if (s.len == 0) return "";
	return s.copy(self.allocator);
}

// Release a String returned by list_* / get_property_safe_string.
fn void CantorDB.free_string(&self, String s)
{
	if (s.len > 0) alloc::free(self.allocator, s.ptr);
}

fn String CantorDB.finish(&self, DString ds)
{
	String out = ds.len() == 0 ? "" : ds.copy_str(self.allocator);
	ds.free();
	return out;
}

fn uint[] CantorDB.ids_of(&self, Bitmap* b) => bitmap_to_array(b, self.allocator);

fn void CantorDB.release_ids(&self, uint[] ids)
{
	if (ids.len > 0) alloc::free(self.allocator, ids.ptr);
}

// no error side effects
fn Set* CantorDB.find_set(&self, String name)
{
	uint? id = self.registry.get_id(name);
	if (catch id) return null;
	Set*? s = self.sets.get(id);
	if (catch s) return null;
	return s;
}

fn Set* CantorDB.need_set(&self, String name)
{
	Set* s = self.find_set(name);
	if (!s) self.fail(ER_SET_NOT_FOUND, string::tformat("Error: Set \"%s\" not found.", name));
	return s;
}

fn Set* CantorDB.set_by_id(&self, uint id)
{
	Set*? s = self.sets.get(id);
	if (catch s) return null;
	return s;
}

// Allocates, charges the memory budget and registers a new set. Null (with the
// database shut off) if that fails.
fn Set* CantorDB.make_set(&self, String name)
{
	Set*? s = build_set(0, "", self.allocator);
	if (catch s)
	{
		self.emergency_shut_off = true;
		self.say("Error: Out of memory.");
		return null;
	}
	self.memory_used += SET_BASE_COST;
	if (self.memory_limit > 0 && self.memory_used > self.memory_limit)
	{
		self.emergency_shut_off = true;
		self.say("Error: Memory limit exceeded.");
		s.destroy(self.allocator);
		return null;
	}
	uint id = self.registry.register(name);
	s.id = id;
	s.set_name = self.registry.get_name(id)!!;
	self.sets.set(id, s);
	return s;
}

// A set for a computed result. A previous cached result with the same name is replaced.
fn Set* CantorDB.make_cache_set(&self, String name)
{
	if (self.registry.has_name(name))
	{
		Set* old = self.find_set(name);
		Set* cache = self.find_set(CACHE_SET);
		if (old && cache && roaring_bitmap_contains(cache.has_element, old.id))
		{
			self.delete_set(name, false);
		}
		else
		{
			self.fail(ER_SET_EX, string::tformat("Error: Set \"%s\" already exists.", name));
			return null;
		}
	}
	return self.make_set(name);
}

// ---- sets ----

fn bool CantorDB.create_set(&self, String set_name, bool gen_set = false)
{
	if (self.shut_off()) return false;
	if (self.registry.has_name(set_name))
	{
		self.fail(ER_SET_EX, "Error: Set name already exists. Use a unique name for every set.");
		return false;
	}
	Set* s = self.make_set(set_name);
	if (!s) return false;
	if (!gen_set) self.add_member(UNIVERSAL_SET, set_name);
	return true;
}

fn bool CantorDB.delete_set(&self, String set_name, bool safe = true)
{
	if (self.shut_off()) return false;
	Set* target = self.need_set(set_name);
	if (!target) return false;

	// Unlink from all parents' has_element
	uint[] parents = self.ids_of(target.member_of);
	defer self.release_ids(parents);
	foreach (pid : parents)
	{
		Set* p = self.set_by_id(pid);
		if (p) roaring_bitmap_remove(p.has_element, target.id);
	}

	// Unlink from all children's member_of
	uint[] children = self.ids_of(target.has_element);
	defer self.release_ids(children);
	foreach (cid : children)
	{
		Set* c = self.set_by_id(cid);
		if (c) roaring_bitmap_remove(c.member_of, target.id);
	}

	// Remove from property indices
	@pool()
	{
		foreach (pid : self.properties.tkeys())
		{
			Set* ps = self.properties.get(pid)!!;
			roaring_bitmap_remove(ps.has_element, target.id);
		}
	};

	Set* trash = self.find_set(TRASH_SET);
	if (safe)
	{
		// Soft delete: keep the set alive in TRASH for restore_set. member_of and
		// has_element on target are preserved as the restore record.
		self.registry.unname(target.id);
		if (trash) roaring_bitmap_add(trash.has_element, target.id);
	}
	else
	{
		// Hard delete: destroy the set permanently
		if (trash) roaring_bitmap_remove(trash.has_element, target.id);
		uint id = target.id;
		(void)self.sets.remove(id);
		target.destroy(self.allocator);
		self.registry.remove(id);
	}
	return true;
}

fn bool CantorDB.restore_set(&self, String set_name)
{
	if (self.shut_off()) return false;

	// Find the set in TRASH by name
	Set* trash = self.find_set(TRASH_SET);
	Set* target = null;
	if (trash)
	{
		uint[] trashed = self.ids_of(trash.has_element);
		defer self.release_ids(trashed);
		foreach (tid : trashed)
		{
			Set* s = self.set_by_id(tid);
			if (s && s.set_name == set_name)
			{
				target = s;
				break;
			}
		}
	}
	if (!target)
	{
		self.fail(ER_SET_NOT_FOUND, string::tformat("Error: Set \"%s\" not found in TRASH.", set_name));
		return false;
	}
	if (!self.registry.rename_back(target.id))
	{
		self.fail(ER_SET_EX, string::tformat("Error: Set \"%s\" already exists.", set_name));
		return false;
	}

	// Remove from TRASH
	roaring_bitmap_remove(trash.has_element, target.id);

	// Re-add to UNIVERSAL
	self.add_member(UNIVERSAL_SET, set_name);

	// Re-link to all parent sets (member_of was preserved as restore record)
	uint[] parents = self.ids_of(target.member_of);
	defer self.release_ids(parents);
	foreach (pid : parents)
	{
		Set* parent = self.set_by_id(pid);
		if (!parent || parent.set_name == UNIVERSAL_SET) continue;   // universal was re-added above
		roaring_bitmap_add(parent.has_element, target.id);
	}

	// Re-link to all child sets (has_element was preserved as restore record)
	uint[] children = self.ids_of(target.has_element);
	defer self.release_ids(children);
	foreach (cid : children)
	{
		Set* child = self.set_by_id(cid);
		if (child) roaring_bitmap_add(child.member_of, target.id);
	}

	// Re-add to property indices
	@pool()
	{
		foreach (key : target.properties.tkeys())
		{
			Set* ps = self.property_set(key);
			if (ps) roaring_bitmap_add(ps.has_element, target.id);
		}
	};
	return true;
}

fn bool CantorDB.add_member(&self, String set_name, String element_name)
{
	if (self.shut_off()) return false;
	Set* parent = self.need_set(set_name);
	if (!parent) return false;
	Set* child = self.need_set(element_name);
	if (!child) return false;
	if (roaring_bitmap_contains(parent.has_element, child.id))
	{
		self.fail(ER_ELEM_ALR_ADDED, string::tformat("Error: \"%s\" is already an element of \"%s\".", element_name, set_name));
		return false;
	}
	roaring_bitmap_add(parent.has_element, child.id);
	roaring_bitmap_add(child.member_of, parent.id);
	return true;
}

// Links an existing set object (e.g. a computed result) under set_name.
fn void CantorDB.add_member_set(&self, String set_name, Set* element)
{
	if (self.emergency_shut_off) return;
	Set* parent = self.find_set(set_name);
	if (!parent) return;
	roaring_bitmap_add(parent.has_element, element.id);
	roaring_bitmap_add(element.member_of, parent.id);
}

fn bool CantorDB.remove_member(&self, String set_name, String element_name)
{
	if (self.shut_off()) return false;
	Set* parent = self.need_set(set_name);
	if (!parent) return false;
	Set* element = self.need_set(element_name);
	if (!element) return false;
	if (!roaring_bitmap_contains(parent.has_element, element.id))
	{
		self.fail(ER_ELEM_NOT_MEM, string::tformat("Error: Element not a member of %s", element_name));
		return false;
	}
	roaring_bitmap_remove(parent.has_element, element.id);
	roaring_bitmap_remove(element.member_of, parent.id);
	return true;
}

fn bool CantorDB.is_element(&self, String set_a_name, String set_b_name)
{
	if (self.shut_off()) return false;
	Set* a = self.need_set(set_a_name);
	if (!a) return false;
	Set* b = self.need_set(set_b_name);
	if (!b) return false;
	return roaring_bitmap_contains(b.has_element, a.id);
}

fn bool CantorDB.rename_set(&self, String set_name, String set_new_name)
{
	if (self.shut_off()) return false;
	Set* s = self.need_set(set_name);
	if (!s) return false;
	if (self.registry.has_name(set_new_name))
	{
		self.fail(ER_SET_EX, string::tformat("Error: Set \"%s\" already exists.", set_new_name));
		return false;
	}
	s.set_name = self.registry.rename(s.id, set_new_name);
	return true;
}

fn bool CantorDB.clear_cache(&self)
{
	if (self.shut_off()) return false;
	Set* cache = self.find_set(CACHE_SET);
	if (!cache) return true;
	// Snapshot the ids first: delete_set modifies has_element while unlinking
	uint[] cached = self.ids_of(cache.has_element);
	defer self.release_ids(cached);
	foreach (id : cached)
	{
		Set* s = self.set_by_id(id);
		if (s) self.delete_set(s.set_name, false);
	}
	return true;
}

// ---- set algebra ----

fn Set* CantorDB.combine(&self, Set* a, Set* b, SetOp op)
{
	if (self.shut_off()) return null;
	if (!a || !b)
	{
		self.fail(ER_SET_NOT_FOUND, "Error: Null set pointer.");
		return null;
	}
	String sym;
	switch (op)
	{
		case UNION: sym = " ∪ ";
		case INTERSECTION: sym = " ∩ ";
		case DIFFERENCE: sym = " - ";
		case SYMMETRIC: sym = " ⊕ ";
	}
	Set* result = self.make_cache_set(string::tformat("%s%s%s", a.set_name, sym, b.set_name));
	if (!result) return null;

	Bitmap* r;
	switch (op)
	{
		case UNION: r = roaring_bitmap_or(a.has_element, b.has_element);
		case INTERSECTION: r = roaring_bitmap_and(a.has_element, b.has_element);
		case DIFFERENCE: r = roaring_bitmap_andnot(a.has_element, b.has_element);
		case SYMMETRIC: r = roaring_bitmap_xor(a.has_element, b.has_element);
	}
	roaring_bitmap_free(result.has_element);
	result.has_element = r;

	self.add_member_set(CACHE_SET, result);
	return result;
}

fn Set* CantorDB.combine_named(&self, String set_a_name, String set_b_name, SetOp op)
{
	if (self.shut_off()) return null;
	Set* a = self.need_set(set_a_name);
	if (!a) return null;
	Set* b = self.need_set(set_b_name);
	if (!b) return null;
	return self.combine(a, b, op);
}

fn Set* CantorDB.set_union(&self, String a, String b) => self.combine_named(a, b, UNION);
fn Set* CantorDB.set_union_set(&self, Set* a, Set* b) => self.combine(a, b, UNION);
fn Set* CantorDB.set_intersection(&self, String a, String b) => self.combine_named(a, b, INTERSECTION);
fn Set* CantorDB.set_intersection_set(&self, Set* a, Set* b) => self.combine(a, b, INTERSECTION);
fn Set* CantorDB.set_difference(&self, String a, String b) => self.combine_named(a, b, DIFFERENCE);
fn Set* CantorDB.set_difference_set(&self, Set* a, Set* b) => self.combine(a, b, DIFFERENCE);
fn Set* CantorDB.symmetric_difference(&self, String a, String b) => self.combine_named(a, b, SYMMETRIC);
fn Set* CantorDB.symmetric_difference_set(&self, Set* a, Set* b) => self.combine(a, b, SYMMETRIC);

fn Set* CantorDB.set_complement(&self, String set_name)
{
	if (self.shut_off()) return null;
	if (!self.need_set(set_name)) return null;
	Set* complement = self.set_difference(UNIVERSAL_SET, set_name);
	if (!complement) return null;
	String new_name = string::tformat("%s'", set_name);
	if (self.registry.has_name(new_name))
	{
		self.fail(ER_SET_EX, string::tformat("Error: Set \"%s\" already exists.", new_name));
		self.delete_set(complement.set_name, false);
		return null;
	}
	complement.set_name = self.registry.rename(complement.id, new_name);
	return complement;
}

// ---- comparisons ----

fn bool CantorDB.is_disjoint(&self, String set_a_name, String set_b_name)
{
	if (self.shut_off()) return false;
	Set* a = self.find_set(set_a_name);
	Set* b = self.find_set(set_b_name);
	if (!a || !b)
	{
		self.fail(ER_SET_NOT_FOUND, "Error: Set not found.");
		return false;
	}
	return self.is_disjoint_set(a, b);
}

fn bool CantorDB.is_disjoint_set(&self, Set* a, Set* b)
{
	if (self.shut_off()) return false;
	if (!a || !b) return false;
	return !roaring_bitmap_intersect(a.has_element, b.has_element);
}

fn bool CantorDB.is_subset(&self, String set_a_name, String set_b_name)
{
	if (self.shut_off()) return false;
	Set* a = self.need_set(set_a_name);
	if (!a) return false;
	Set* b = self.need_set(set_b_name);
	if (!b) return false;
	return self.is_subset_set(a, b);
}

fn bool CantorDB.is_subset_set(&self, Set* a, Set* b)
{
	if (self.shut_off()) return false;
	if (!a || !b)
	{
		self.fail(ER_SET_NOT_FOUND, "Error: Null set pointer.");
		return false;
	}
	return roaring_bitmap_is_subset(a.has_element, b.has_element);
}

fn bool CantorDB.is_superset(&self, String a, String b)
{
	if (self.shut_off()) return false;
	return self.is_subset(b, a);
}

fn bool CantorDB.is_superset_set(&self, Set* a, Set* b)
{
	if (self.shut_off()) return false;
	return self.is_subset_set(b, a);
}

fn bool CantorDB.is_equal(&self, String set_a_name, String set_b_name)
{
	if (self.shut_off()) return false;
	Set* a = self.find_set(set_a_name);
	Set* b = self.find_set(set_b_name);
	if (!a || !b)
	{
		self.fail(ER_SET_NOT_FOUND, "Error: Set not found.");
		return false;
	}
	return self.is_equal_set(a, b);
}

fn bool CantorDB.is_equal_set(&self, Set* a, Set* b)
{
	if (self.shut_off()) return false;
	if (!a || !b) return false;
	return roaring_bitmap_equals(a.has_element, b.has_element);
}

fn bool CantorDB.is_proper_subset(&self, String set_a_name, String set_b_name)
{
	if (self.shut_off()) return false;
	Set* a = self.find_set(set_a_name);
	Set* b = self.find_set(set_b_name);
	if (!a || !b)
	{
		self.fail(ER_SET_NOT_FOUND, "Error: Set not found.");
		return false;
	}
	return self.is_proper_subset_set(a, b);
}

fn bool CantorDB.is_proper_subset_set(&self, Set* a, Set* b)
{
	if (self.shut_off()) return false;
	if (!a || !b) return false;
	return roaring_bitmap_is_strict_subset(a.has_element, b.has_element);
}

// ---- properties ----

// The set whose has_element holds every set that has this property.
fn Set* CantorDB.property_set(&self, String property_name)
{
	uint? pid = self.property_registry.get_id(property_name);
	if (catch pid) return null;
	Set*? s = self.properties.get(pid);
	if (catch s) return null;
	return s;
}

fn bool CantorDB.register_property(&self, String property_name, PropType type)
{
	uint pid = self.property_registry.register(property_name);
	String owned = self.property_registry.get_name(pid)!!;
	Set*? ps = build_set(pid, owned, self.allocator);
	if (catch ps)
	{
		self.property_registry.remove(pid);
		self.emergency_shut_off = true;
		self.say("Error: Out of memory.");
		return false;
	}
	self.properties.set(pid, ps);
	self.property_types.set(pid, type);
	return true;
}

fn bool CantorDB.create_property_typed(&self, String property_name, PropType type)
{
	if (self.shut_off()) return false;
	if (self.property_registry.has_name(property_name))
	{
		self.fail(ER_KEY_EX, string::tformat("Error: Property \"%s\" already registered.", property_name));
		return false;
	}
	return self.register_property(property_name, type);
}

// type: int | integer | string | double | bool | long
fn bool CantorDB.create_property(&self, String property_name, String type)
{
	if (self.shut_off()) return false;
	if (self.property_registry.has_name(property_name))
	{
		self.fail(ER_KEY_EX, string::tformat("Error: Property \"%s\" already registered.", property_name));
		return false;
	}
	PropType t;
	switch (type)
	{
		case "int":
		case "integer": t = INTEGER;
		case "string": t = STRING;
		case "double": t = DOUBLE;
		case "bool": t = BOOL;
		case "long": t = LONG;
		default:
			self.fail(ER_KEY_WRONG_TYPE, string::tformat("Error: Unknown type \"%s\". Use int, string, double, bool, or long.", type));
			return false;
	}
	return self.register_property(property_name, t);
}

fn PropType? CantorDB.property_type(&self, String property_name)
{
	uint pid = self.property_registry.get_id(property_name)!;
	return self.property_types.get(pid);
}

// Stores value on the set (taking a copy of any string) and indexes the set under the property.
fn void CantorDB.attach_property(&self, Set* s, String property_name, PropertyValue value)
{
	uint pid = self.property_registry.get_id(property_name)!!;
	String key = self.property_registry.get_name(pid)!!;
	PropertyValue stored = value;
	if (stored.type == STRING) stored.value_string = self.dup(value.value_string);
	s.properties.set(key, stored);
	Set* ps = self.properties.get(pid)!!;
	roaring_bitmap_add(ps.has_element, s.id);
}

// Adds the property to the set with the type's default value.
fn bool CantorDB.add_property(&self, String property_name, String set_name)
{
	if (self.shut_off()) return false;
	Set* s = self.need_set(set_name);
	if (!s) return false;
	if (s.properties.has_key(property_name))
	{
		self.fail(ER_KEY_EX, string::tformat("Error: Key \"%s\" already exists as a property on this set.", property_name));
		return false;
	}
	PropType? t = self.property_type(property_name);
	if (catch t)
	{
		self.fail(ER_KEY_NOT_FOUND, string::tformat("Error: Property \"%s\" has not been created. Use create_property first.", property_name));
		return false;
	}
	self.attach_property(s, property_name, pv_default(t));
	return true;
}

fn bool CantorDB.add_property_value(&self, String key, PropertyValue value, String set_name)
{
	if (self.shut_off()) return false;
	Set* s = self.need_set(set_name);
	if (!s) return false;
	PropType? t = self.property_type(key);
	if (catch t)
	{
		self.fail(ER_KEY_NOT_FOUND, string::tformat("Error: Property \"%s\" has not been created. Use create_property first.", key));
		return false;
	}
	if (t != value.type)
	{
		self.fail(ER_KEY_WRONG_TYPE, string::tformat("Error: Property \"%s\" is not %s type.", key, type_name(value.type)));
		return false;
	}
	if (s.properties.has_key(key))
	{
		self.fail(ER_KEY_EX, string::tformat("Error: Key \"%s\" already exists as a property.", key));
		return false;
	}
	self.attach_property(s, key, value);
	if (value.type == STRING)
	{
		self.memory_used += key.len + value.value_string.len + 64;
		if (self.memory_limit > 0 && self.memory_used > self.memory_limit)
		{
			self.emergency_shut_off = true;
			self.say("Error: Memory limit exceeded.");
			return false;
		}
	}
	return true;
}

// Looks up set + key; sets the same errors as the C++ on failure.
fn PropertyValue* CantorDB.property_ref(&self, String set_name, String key_name)
{
	Set* s = self.need_set(set_name);
	if (!s) return null;
	PropertyValue*? pv = s.properties.get_ref(key_name);
	if (catch pv)
	{
		self.fail(ER_KEY_NOT_FOUND, string::tformat("Error: Key \"%s\" not found in set \"%s\".", key_name, set_name));
		return null;
	}
	return pv;
}

// On any error ec is set and a bool false is returned (check ec).
fn PropertyValue CantorDB.get_property(&self, String set_name, String key_name)
{
	if (self.shut_off()) return pv_bool(false);
	PropertyValue* pv = self.property_ref(set_name, key_name);
	if (!pv) return pv_bool(false);
	return *pv;
}

fn PropertyValue* CantorDB.typed_property(&self, String set_name, String key_name, PropType expected)
{
	PropertyValue* pv = self.property_ref(set_name, key_name);
	if (!pv) return null;
	if (pv.type != expected)
	{
		self.fail(ER_KEY_WRONG_TYPE, string::tformat("Error: Key \"%s\" is not %s type.", key_name, type_name(expected)));
		return null;
	}
	return pv;
}

fn int CantorDB.get_property_safe_int(&self, String set_name, String key_name)
{
	if (self.shut_off()) return -1;
	PropertyValue* pv = self.typed_property(set_name, key_name, INTEGER);
	return pv ? pv.value_int : 0;
}

// The returned String is a view into the database: valid until the property is updated or deleted.
fn String CantorDB.get_property_safe_string(&self, String set_name, String key_name)
{
	if (self.emergency_shut_off) return "Error: Database emergency shut off.";
	PropertyValue* pv = self.typed_property(set_name, key_name, STRING);
	return pv ? pv.value_string : "";
}

fn double CantorDB.get_property_safe_double(&self, String set_name, String key_name)
{
	if (self.shut_off()) return 0.0;
	PropertyValue* pv = self.typed_property(set_name, key_name, DOUBLE);
	return pv ? pv.value_double : 0.0;
}

fn bool CantorDB.get_property_safe_bool(&self, String set_name, String key_name)
{
	if (self.shut_off()) return false;
	PropertyValue* pv = self.typed_property(set_name, key_name, BOOL);
	return pv ? pv.value_bool : false;
}

fn long CantorDB.get_property_safe_long(&self, String set_name, String key_name)
{
	if (self.shut_off()) return 0;
	PropertyValue* pv = self.typed_property(set_name, key_name, LONG);
	return pv ? pv.value_long : 0;
}

fn bool CantorDB.delete_property(&self, String set_name, String key_name)
{
	if (self.shut_off()) return false;
	Set* s = self.need_set(set_name);
	if (!s) return false;
	PropertyValue*? pv = s.properties.get_ref(key_name);
	if (catch pv)
	{
		self.fail(ER_KEY_NOT_FOUND, string::tformat("Error: Key \"%s\" not found in set \"%s\".", key_name, set_name));
		return false;
	}
	if (pv.type == STRING && pv.value_string.len > 0) alloc::free(self.allocator, pv.value_string.ptr);
	(void)s.properties.remove(key_name);
	Set* ps = self.property_set(key_name);
	if (ps) roaring_bitmap_remove(ps.has_element, s.id);
	return true;
}

fn bool CantorDB.update_property(&self, String set_name, String key_name, PropertyValue value)
{
	if (self.shut_off()) return false;
	PropertyValue* pv = self.property_ref(set_name, key_name);
	if (!pv) return false;
	if (pv.type != value.type)
	{
		self.fail(ER_KEY_WRONG_TYPE, string::tformat("Error: Key \"%s\" is not %s type.", key_name, type_name(value.type)));
		return false;
	}
	if (value.type == STRING)
	{
		String fresh = self.dup(value.value_string);
		if (pv.value_string.len > 0) alloc::free(self.allocator, pv.value_string.ptr);
		pv.value_string = fresh;
	}
	else
	{
		*pv = value;
	}
	return true;
}

// ---- listing ----

fn String[] CantorDB.sorted_keys(&self, Set* s, bool only_type, PropType type)
{
	String[] all = s.properties.keys(self.allocator);
	if (only_type)
	{
		usz n = 0;
		foreach (k : all)
		{
			PropertyValue pv = s.properties.get(k)!!;
			if (pv.type == type) all[n++] = k;
		}
		all = all[:n];
	}
	sort_strings(all);
	return all;
}

fn void CantorDB.release_keys(&self, String[] keys)
{
	if (keys.len > 0) alloc::free(self.allocator, keys.ptr);
}

fn String CantorDB.list_all_keys(&self)
{
	if (self.emergency_shut_off) return self.dup("Error: Database emergency shut off.");
	String[] names = self.property_registry.name_to_id.keys(self.allocator);
	defer self.release_keys(names);
	sort_strings(names);
	DString ds = dstring::new_with_capacity(self.allocator, 128);
	foreach (name : names)
	{
		PropType t = self.property_type(name)!!;
		if (ds.len() > 0) ds.append_char('\n');
		ds.append_string(string::tformat("%s: %s", name, type_name(t)));
	}
	return self.finish(ds);
}

fn String CantorDB.list_property_keys(&self, String set_name)
{
	if (self.emergency_shut_off) return self.dup("Error: Database emergency shut off.");
	Set* s = self.need_set(set_name);
	if (!s) return "";
	String[] keys = self.sorted_keys(s, false, INTEGER);
	defer self.release_keys(keys);
	DString ds = dstring::new_with_capacity(self.allocator, 128);
	foreach (key : keys)
	{
		PropertyValue pv = s.properties.get(key)!!;
		if (ds.len() > 0) ds.append_char('\n');
		ds.append_string(string::tformat("%s: %s", key, type_name(pv.type)));
	}
	return self.finish(ds);
}

fn String CantorDB.list_properties(&self, String set_name)
{
	if (self.emergency_shut_off) return self.dup("Error: Database emergency shut off.");
	Set* s = self.need_set(set_name);
	if (!s) return "";
	String[] keys = self.sorted_keys(s, false, INTEGER);
	defer self.release_keys(keys);
	DString ds = dstring::new_with_capacity(self.allocator, 128);
	foreach (key : keys)
	{
		PropertyValue pv = s.properties.get(key)!!;
		if (ds.len() > 0) ds.append_char('\n');
		ds.append_string(string::tformat("%s:%s:%s", key, value_to_string(pv), type_name(pv.type)));
	}
	return self.finish(ds);
}

fn String CantorDB.list_properties_of_type(&self, String set_name, PropType type)
{
	if (self.emergency_shut_off) return self.dup("Error: Database emergency shut off.");
	Set* s = self.need_set(set_name);
	if (!s) return "";
	String[] keys = self.sorted_keys(s, true, type);
	defer self.release_keys(keys);
	DString ds = dstring::new_with_capacity(self.allocator, 128);
	foreach (key : keys)
	{
		PropertyValue pv = s.properties.get(key)!!;
		if (ds.len() > 0) ds.append_char('\n');
		ds.append_string(string::tformat("%s:%s", key, value_to_string(pv)));
	}
	return self.finish(ds);
}

fn String CantorDB.list_string_properties(&self, String set_name) => self.list_properties_of_type(set_name, STRING);
fn String CantorDB.list_int_properties(&self, String set_name) => self.list_properties_of_type(set_name, INTEGER);
fn String CantorDB.list_double_properties(&self, String set_name) => self.list_properties_of_type(set_name, DOUBLE);
fn String CantorDB.list_bool_properties(&self, String set_name) => self.list_properties_of_type(set_name, BOOL);
fn String CantorDB.list_long_properties(&self, String set_name) => self.list_properties_of_type(set_name, LONG);

fn String CantorDB.names_of(&self, Bitmap* b, bool skip_universal)
{
	uint[] ids = self.ids_of(b);
	defer self.release_ids(ids);
	DString ds = dstring::new_with_capacity(self.allocator, 128);
	foreach (id : ids)
	{
		Set* e = self.set_by_id(id);
		if (!e) continue;
		if (skip_universal && e.set_name == UNIVERSAL_SET) continue;
		if (ds.len() > 0) ds.append_char('\n');
		ds.append_string(e.set_name);
	}
	return self.finish(ds);
}

fn String CantorDB.list_elements(&self, String set_name)
{
	if (self.emergency_shut_off) return self.dup("Error: Database emergency shut off.");
	Set* s = self.need_set(set_name);
	if (!s) return "";
	return self.names_of(s.has_element, false);
}

fn String CantorDB.list_sets(&self, String set_name)
{
	if (self.emergency_shut_off) return self.dup("Error: Database emergency shut off.");
	Set* s = self.need_set(set_name);
	if (!s) return "";
	return self.names_of(s.member_of, true);
}

fn int CantorDB.get_cardinality(&self, String set_name)
{
	if (self.shut_off()) return -1;
	Set* s = self.need_set(set_name);
	if (!s) return -1;
	return (int)roaring_bitmap_get_cardinality(s.has_element);
}

fn String CantorDB.list_all_sets(&self)
{
	if (self.emergency_shut_off) return self.dup("Error: Database emergency shut off.");
	Set* s = self.find_set(UNIVERSAL_SET);
	return s ? self.names_of(s.has_element, false) : "";
}

fn String CantorDB.list_cached_sets(&self)
{
	if (self.emergency_shut_off) return self.dup("Error: Database emergency shut off.");
	Set* s = self.find_set(CACHE_SET);
	return s ? self.names_of(s.has_element, false) : "";
}

// ---- queries ----

// Elements of set_name whose property compares true against value. The type of value
// picks the property type (int, long, double; string and bool support EQUAL only).
fn Set* CantorDB.where_elements(&self, String set_name, String property, CmpOp op, PropertyValue value)
{
	if (self.shut_off()) return null;
	Set* parent = self.need_set(set_name);
	if (!parent) return null;

	bool ordered = value.type == INTEGER || value.type == LONG || value.type == DOUBLE;
	if (op != EQUAL && !ordered)
	{
		self.fail(ER_KEY_WRONG_TYPE, string::tformat("Error: Ordering comparison is not supported for %s properties.", type_name(value.type)));
		return null;
	}

	Set* ps = self.property_set(property);
	bool indexed = false;
	if (ps)
	{
		PropType? t = self.property_type(property);
		if (try t) indexed = t == value.type && roaring_bitmap_get_cardinality(ps.has_element) > 0;
	}
	if (!indexed)
	{
		self.fail(ER_KEY_NOT_FOUND, string::tformat("Error: No %s property \"%s\" in index.", type_word(value.type), property));
		return null;
	}

	String sym = op == GREATER ? ">" : (op == LESSER ? "<" : "==");
	Set* result = self.make_cache_set(string::tformat("%s[%s %s %s]", set_name, property, sym, value_to_string(value)));
	if (!result) return null;

	Bitmap* candidates = roaring_bitmap_and(parent.has_element, ps.has_element);
	uint[] ids = self.ids_of(candidates);
	roaring_bitmap_free(candidates);
	defer self.release_ids(ids);
	foreach (id : ids)
	{
		Set* e = self.set_by_id(id);
		if (!e) continue;
		PropertyValue* pv = e.properties.get_ref(property)!!;
		bool hit;
		switch (value.type)
		{
			case INTEGER: hit = cmp_op(op, pv.value_int, value.value_int);
			case LONG: hit = cmp_op(op, pv.value_long, value.value_long);
			case DOUBLE: hit = cmp_op(op, pv.value_double, value.value_double);
			case STRING: hit = pv.value_string == value.value_string;
			case BOOL: hit = pv.value_bool == value.value_bool;
			default: hit = false;
		}
		if (hit) roaring_bitmap_add(result.has_element, id);
	}

	self.add_member_set(CACHE_SET, result);
	return result;
}

fn Set* CantorDB.where_elements_greater_than(&self, String set_name, String property, PropertyValue value) => self.where_elements(set_name, property, GREATER, value);
fn Set* CantorDB.where_elements_lesser_than(&self, String set_name, String property, PropertyValue value) => self.where_elements(set_name, property, LESSER, value);
fn Set* CantorDB.where_elements_equal_than(&self, String set_name, String property, PropertyValue value) => self.where_elements(set_name, property, EQUAL, value);

// ---- binary save / load (same file layout as the C++ version, little endian) ----

struct Writer
{
	List{char} buf;
}

fn void Writer.u8(&self, char v) => self.buf.push(v);

fn void Writer.u32(&self, uint v)
{
	for (int i = 0; i < 4; i++) self.buf.push((char)((v >> (8 * i)) & 0xFF));
}

fn void Writer.u64(&self, ulong v)
{
	for (int i = 0; i < 8; i++) self.buf.push((char)((v >> (8 * i)) & 0xFF));
}

fn void Writer.str(&self, String s)
{
	self.u32((uint)s.len);
	foreach (c : s) self.buf.push(c);
}

struct Reader
{
	char[] data;
	usz pos;
	bool ok;
}

fn bool Reader.need(&self, usz n)
{
	if (!self.ok || self.pos + n > self.data.len)
	{
		self.ok = false;
		return false;
	}
	return true;
}

fn char Reader.u8(&self)
{
	if (!self.need(1)) return 0;
	return self.data[self.pos++];
}

fn uint Reader.u32(&self)
{
	if (!self.need(4)) return 0;
	uint v = 0;
	for (int i = 0; i < 4; i++) v |= (uint)self.data[self.pos++] << (8 * i);
	return v;
}

fn ulong Reader.u64(&self)
{
	if (!self.need(8)) return 0;
	ulong v = 0;
	for (int i = 0; i < 8; i++) v |= (ulong)self.data[self.pos++] << (8 * i);
	return v;
}

// Heap copy owned by the caller; "" on error (same limits as the C++ reader).
fn String Reader.str(&self, Allocator heap)
{
	uint len = self.u32();
	if (!self.ok || len > 10000000) return "";
	if (!self.need(len)) return "";
	String s = ((String)self.data[self.pos:len]).copy(heap);
	self.pos += len;
	return s;
}

fn void write_properties(Writer* w, Set* s, CantorDB* db)
{
	w.u32((uint)s.properties.len());
	// grouped by type like the C++ (int, string, double, bool, long, date), sorted by key within a group
	for (int tag = 0; tag <= 5; tag++)
	{
		PropType t = type_from_tag(tag)!!;
		String[] keys = db.sorted_keys(s, true, t);
		defer db.release_keys(keys);
		foreach (key : keys)
		{
			PropertyValue pv = s.properties.get(key)!!;
			w.str(key);
			w.u8((char)t.ordinal);
			switch (t)
			{
				case INTEGER: w.u32((uint)pv.value_int);
				case STRING: w.str(pv.value_string);
				case DOUBLE: w.u64(bitcast(pv.value_double, ulong));
				case BOOL: w.u8(pv.value_bool ? 1 : 0);
				case LONG: w.u64((ulong)pv.value_long);
				case DATE: w.u64((ulong)pv.value_date);
			}
		}
	}
}

// s may be null (value is read and discarded) when the set could not be created.
fn void read_properties(Reader* r, CantorDB* db, Set* s)
{
	uint count = r.u32();
	for (uint i = 0; i < count && r.ok; i++)
	{
		String key = r.str(db.allocator);
		defer db.free_string(key);
		int tag = r.u8();
		PropType? t = type_from_tag(tag);
		if (catch t)
		{
			r.ok = false;
			return;
		}
		PropertyValue pv;
		switch (t)
		{
			case INTEGER: pv = pv_int((int)r.u32());
			case STRING: pv = pv_string(r.str(db.allocator));
			case DOUBLE: pv = pv_double(bitcast(r.u64(), double));
			case BOOL: pv = pv_bool(r.u8() != 0);
			case LONG: pv = pv_long((long)r.u64());
			case DATE: pv = pv_date((long)r.u64());
		}
		if (!r.ok) return;
		if (s)
		{
			if (!db.property_registry.has_name(key)) db.register_property(key, t);
			db.attach_property(s, key, pv);
		}
		if (t == STRING) db.free_string(pv.value_string);
	}
}

fn bool save_cantordb(CantorDB* db, String path)
{
	Writer w;
	w.buf.init(db.allocator);
	defer w.buf.free();

	w.str(db.name);

	// Sets to save (everything by name except UNIVERSAL_SET and CACHE_SET), sorted by name
	String[] names = db.registry.name_to_id.keys(db.allocator);
	defer db.release_keys(names);
	sort_strings(names);
	List{Set*} sets_to_save;
	sets_to_save.init(db.allocator);
	defer sets_to_save.free();
	foreach (name : names)
	{
		if (name == UNIVERSAL_SET || name == CACHE_SET) continue;
		sets_to_save.push(db.find_set(name));
	}

	w.u32((uint)sets_to_save.len());
	foreach (s : sets_to_save)
	{
		w.str(s.set_name);
		write_properties(&w, s, db);
	}

	// membership relationships (parent -> child, skipping UNIVERSAL_SET and CACHE_SET)
	List{String} parents;
	List{String} children;
	parents.init(db.allocator);
	children.init(db.allocator);
	defer parents.free();
	defer children.free();
	foreach (s : sets_to_save)
	{
		uint[] ids = db.ids_of(s.has_element);
		defer db.release_ids(ids);
		foreach (id : ids)
		{
			Set* child = db.set_by_id(id);
			if (!child || child.set_name == UNIVERSAL_SET || child.set_name == CACHE_SET) continue;
			parents.push(s.set_name);
			children.push(child.set_name);
		}
	}
	w.u32((uint)parents.len());
	foreach (i, p : parents)
	{
		w.str(p);
		w.str(children[i]);
	}

	// property type registry
	String[] props = db.property_registry.name_to_id.keys(db.allocator);
	defer db.release_keys(props);
	sort_strings(props);
	w.u32((uint)props.len);
	foreach (p : props)
	{
		w.str(p);
		w.u8((char)db.property_type(p)!!.ordinal);
	}

	String tmp_path = string::tformat("%s.tmp", path);
	if (catch file::save(tmp_path, w.buf.array_view())) return false;
	ZString tmp_z = string::tformat_zstr("%s.tmp", path);
	ZString path_z = string::tformat_zstr("%s", path);
	libc::remove(path_z);
	libc::rename(tmp_z, path_z);
	return true;
}

// Returns null if the file is missing or damaged. Free with delete_cantordb.
fn CantorDB* load_cantordb(String path, Allocator allocator = mem)
{
	char[]? data = file::load(allocator, path);
	if (catch data) return null;
	defer alloc::free(allocator, data.ptr);

	Reader r = { .data = data, .pos = 0, .ok = true };
	String db_name = r.str(allocator);
	CantorDB* db = new_cantordb(db_name, 0, allocator);
	db.free_string(db_name);

	// sets
	uint set_count = r.u32();
	for (uint i = 0; i < set_count && r.ok; i++)
	{
		String set_name = r.str(allocator);
		defer db.free_string(set_name);
		db.create_set(set_name);
		read_properties(&r, db, db.find_set(set_name));
	}

	// memberships
	uint mem_count = r.u32();
	for (uint i = 0; i < mem_count && r.ok; i++)
	{
		String parent = r.str(allocator);
		String child = r.str(allocator);
		defer db.free_string(parent);
		defer db.free_string(child);
		db.add_member(parent, child);
	}

	if (!r.ok)
	{
		delete_cantordb(db);
		return null;
	}

	// property type registry (absent in older files)
	if (r.pos + 4 <= r.data.len)
	{
		uint prop_count = r.u32();
		for (uint i = 0; i < prop_count && r.ok; i++)
		{
			String prop_name = r.str(allocator);
			defer db.free_string(prop_name);
			int tag = r.u8();
			PropType? t = type_from_tag(tag);
			if (try t)
			{
				if (!db.property_registry.has_name(prop_name)) db.register_property(prop_name, t);
			}
		}
	}
	return db;
}
