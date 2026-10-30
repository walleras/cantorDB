module smoke;
import std::io;
import cantordb;

fn void show(CantorDB* db, String label, String text)
{
	io::printfn("%s: [%s]", label, text);
	db.free_string(text);
}

fn int main()
{
	CantorDB* db = cantordb::new_cantordb("test");
	assert(db.create_set("A"));
	assert(db.create_set("B"));
	assert(db.create_set("C"));
	assert(!db.create_set("A"));
	io::printfn("dup err: %s (ec=%s)", db.error_message, db.ec);
	assert(db.add_member("A", "B"));
	assert(db.add_member("A", "C"));
	assert(!db.add_member("A", "C"));
	io::printfn("dup member err: %s", db.error_message);
	assert(db.add_member("B", "C"));
	show(db, "elements of A", db.list_elements("A"));
	show(db, "sets of C", db.list_sets("C"));
	show(db, "all sets", db.list_all_sets());
	io::printfn("card A = %d", db.get_cardinality("A"));

	cantordb::Set* u = db.set_union("A", "B");
	io::printfn("union name: %s", u.set_name);
	show(db, "union elems", db.list_elements(u.set_name));
	cantordb::Set* i = db.set_intersection("A", "B");
	show(db, "inter elems", db.list_elements(i.set_name));
	cantordb::Set* d = db.set_difference("A", "B");
	show(db, "diff elems", db.list_elements(d.set_name));
	cantordb::Set* x = db.symmetric_difference("A", "B");
	show(db, "symdiff elems", db.list_elements(x.set_name));
	show(db, "cached", db.list_cached_sets());
	cantordb::Set* comp = db.set_complement("A");
	io::printfn("complement name: %s", comp.set_name);
	show(db, "complement elems", db.list_elements(comp.set_name));
	assert(db.is_subset("B", "A"));
	assert(db.is_proper_subset("B", "A"));
	assert(!db.is_subset("A", "B"));
	assert(db.is_superset("A", "B"));
	assert(db.is_disjoint("B", "__trash__"));
	assert(db.is_element("B", "A"));
	assert(db.clear_cache());
	show(db, "cached after clear", db.list_cached_sets());

	// properties
	assert(db.create_property("age", "int"));
	assert(db.create_property("name", "string"));
	assert(db.create_property("score", "double"));
	assert(db.create_property("big", "long"));
	assert(db.create_property("ok", "bool"));
	assert(!db.create_property("zzz", "float"));
	io::printfn("bad type err: %s", db.error_message);
	assert(db.add_property_value("age", cantordb::pv_int(30), "B"));
	assert(db.add_property_value("age", cantordb::pv_int(40), "C"));
	assert(db.add_property_value("name", cantordb::pv_string("bob"), "B"));
	assert(db.add_property_value("score", cantordb::pv_double(1.5), "B"));
	assert(db.add_property_value("big", cantordb::pv_long(9000000000), "B"));
	assert(db.add_property_value("ok", cantordb::pv_bool(true), "B"));
	assert(db.add_property("age", "A"));
	assert(!db.add_property_value("age", cantordb::pv_string("x"), "A"));
	io::printfn("wrong type err: %s", db.error_message);
	show(db, "props B", db.list_properties("B"));
	show(db, "keys B", db.list_property_keys("B"));
	show(db, "int props B", db.list_int_properties("B"));
	show(db, "all keys", db.list_all_keys());
	io::printfn("age of B = %d, name = %s", db.get_property_safe_int("B", "age"), db.get_property_safe_string("B", "name"));
	assert(db.update_property("B", "age", cantordb::pv_int(35)));
	assert(db.update_property("B", "name", cantordb::pv_string("alice")));
	io::printfn("age of B = %d, name = %s", db.get_property_safe_int("B", "age"), db.get_property_safe_string("B", "name"));

	cantordb::Set* w = db.where_elements_greater_than("A", "age", cantordb::pv_int(30));
	io::printfn("where name: %s", w.set_name);
	show(db, "age > 30 in A", db.list_elements(w.set_name));
	cantordb::Set* w2 = db.where_elements_equal_than("A", "name", cantordb::pv_string("alice"));
	show(db, "name == alice", db.list_elements(w2.set_name));
	cantordb::Set* w3 = db.where_elements_lesser_than("A", "age", cantordb::pv_long(5));
	io::printfn("long where on int prop -> %s", w3 == null ? db.error_message : "??");

	// soft delete / restore
	assert(db.delete_set("B"));
	show(db, "all sets after delete B", db.list_all_sets());
	assert(!db.delete_set("B"));
	io::printfn("delete again err: %s", db.error_message);
	assert(db.create_set("B"));   // name is free again
	assert(db.delete_set("B", false));
	assert(db.restore_set("B"));          // the original B is still in the trash
	show(db, "B restored: props", db.list_properties("B"));
	show(db, "B restored: sets of B", db.list_sets("B"));
	show(db, "B restored: elems of A", db.list_elements("A"));
	assert(!db.restore_set("B"));         // no longer in the trash
	io::printfn("restore err: %s", db.error_message);
	assert(db.delete_set("C"));
	assert(db.restore_set("C"));
	show(db, "all sets after restore C", db.list_all_sets());
	show(db, "sets of C", db.list_sets("C"));
	assert(db.rename_set("C", "C2"));
	show(db, "all sets after rename", db.list_all_sets());

	// save / load round trip
	assert(cantordb::save_cantordb(db, "build/cantor_smoke.cdb"));
	CantorDB* db2 = cantordb::load_cantordb("build/cantor_smoke.cdb");
	assert(db2 != null);
	show(db2, "loaded all sets", db2.list_all_sets());
	show(db2, "loaded props C2", db2.list_properties("C2"));
	show(db2, "loaded keys", db2.list_all_keys());
	show(db2, "loaded A elems", db2.list_elements("A"));
	cantordb::delete_cantordb(db2);

	cantordb::delete_cantordb(db);
	io::printn("DONE");
	return 0;
}
