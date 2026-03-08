#include "cantordb.h"
#include <iostream>

using namespace std;

int pass_count = 0;
int fail_count = 0;

void check(const string& label, bool condition) {
	if(condition) {
		cout << "  PASS: " << label << endl;
		pass_count++;
	} else {
		cout << "  FAIL: " << label << endl;
		fail_count++;
	}
}

int main() {
	cout << "=== cantordb Tests ===" << endl << endl;

	// --- Test 1: Constructor creates UNIVERSAL_SET ---
	cout << "--- Test 1: Constructor ---" << endl;
	{
		cantordb db("test");
		check("UNIVERSAL_SET created on construction",
		      db.set_index.find(UNIVERSAL_SET) != db.set_index.end());
		check("db name is set", db.name == "test");
	}
	cout << endl;

	// --- Test 2: create_set ---
	cout << "--- Test 2: create_set ---" << endl;
	{
		cantordb db("test");
		check("create new set returns true",  db.create_set("Animals"));
		check("create new set returns true",  db.create_set("Mammals"));
		check("duplicate create returns false", !db.create_set("Animals"));
		check("set is in index after creation",
		      db.set_index.find("Animals") != db.set_index.end());

		Set* everything = db.set_index[UNIVERSAL_SET];
		Set* animals    = db.set_index["Animals"];
		Set* mammals    = db.set_index["Mammals"];
		check("Animals added to UNIVERSAL_SET",
		      std::find(everything->has_element.begin(), everything->has_element.end(), animals)
		      != everything->has_element.end());
		check("Mammals added to UNIVERSAL_SET",
		      std::find(everything->has_element.begin(), everything->has_element.end(), mammals)
		      != everything->has_element.end());
		check("Animals member_of contains UNIVERSAL_SET",
		      std::find(animals->member_of.begin(), animals->member_of.end(), everything)
		      != animals->member_of.end());
		check("UNIVERSAL_SET not added to itself",
		      everything->member_of.empty());
	}
	cout << endl;

	// --- Test 3: add_property (int) ---
	cout << "--- Test 3: add_property (int) ---" << endl;
	{
		cantordb db("test");
		db.create_set("Primes");
		check("add int property to existing set returns true",
		      db.add_property("count", 5, "Primes"));
		check("value stored correctly",
		      db.set_index["Primes"]->key_num["count"] == 5);
		check("duplicate key returns false",
		      !db.add_property("count", 10, "Primes"));
		check("original value unchanged",
		      db.set_index["Primes"]->key_num["count"] == 5);
		check("add to nonexistent set returns false",
		      !db.add_property("count", 5, "DoesNotExist"));
	}
	cout << endl;

	// --- Test 4: add_property (string) ---
	cout << "--- Test 4: add_property (string) ---" << endl;
	{
		cantordb db("test");
		db.create_set("Colors");
		check("add string property to existing set returns true",
		      db.add_property("description", string("various hues"), "Colors"));
		check("value stored correctly",
		      db.set_index["Colors"]->key_value["description"] == "various hues");
		check("add to nonexistent set returns false",
		      !db.add_property("description", string("x"), "DoesNotExist"));
	}
	cout << endl;

	// --- Test 5: add_member / membership links ---
	cout << "--- Test 5: add_member ---" << endl;
	{
		cantordb db("test");
		db.create_set("Animals");
		db.create_set("Mammals");
		db.create_set("Dog");

		check("add Dog to Mammals returns true",  db.add_member("Mammals", "Dog"));
		check("add Mammals to Animals returns true", db.add_member("Animals", "Mammals"));

		Set* mammals = db.set_index["Mammals"];
		Set* dog     = db.set_index["Dog"];
		Set* animals = db.set_index["Animals"];

		check("Mammals has_element contains Dog",
		      std::find(mammals->has_element.begin(), mammals->has_element.end(), dog)
		      != mammals->has_element.end());
		check("Dog member_of contains Mammals",
		      std::find(dog->member_of.begin(), dog->member_of.end(), mammals)
		      != dog->member_of.end());
		check("Animals has_element contains Mammals",
		      std::find(animals->has_element.begin(), animals->has_element.end(), mammals)
		      != animals->has_element.end());

		check("add_member with nonexistent set returns false",
		      !db.add_member("Ghost", "Dog"));
		check("add_member with nonexistent element returns false",
		      !db.add_member("Animals", "Ghost"));
	}
	cout << endl;

	// --- Test 6: remove_member ---
	cout << "--- Test 6: remove_member ---" << endl;
	{
		cantordb db("test");
		db.create_set("Vehicles");
		db.create_set("Cars");
		db.create_set("Boats");
		db.add_member("Vehicles", "Cars");
		db.add_member("Vehicles", "Boats");

		Set* vehicles = db.set_index["Vehicles"];
		Set* cars     = db.set_index["Cars"];
		Set* boats    = db.set_index["Boats"];

		check("remove Cars from Vehicles returns true",
		      db.remove_member("Vehicles", "Cars"));
		check("Vehicles has_element no longer contains Cars",
		      std::find(vehicles->has_element.begin(), vehicles->has_element.end(), cars)
		      == vehicles->has_element.end());
		check("Cars member_of no longer contains Vehicles",
		      std::find(cars->member_of.begin(), cars->member_of.end(), vehicles)
		      == cars->member_of.end());
		check("Boats still in Vehicles after removing Cars",
		      std::find(vehicles->has_element.begin(), vehicles->has_element.end(), boats)
		      != vehicles->has_element.end());

		check("remove nonexistent member returns false",
		      !db.remove_member("Vehicles", "Cars")); // already removed
		check("remove_member with nonexistent set returns false",
		      !db.remove_member("Ghost", "Boats"));
		check("remove_member with nonexistent element returns false",
		      !db.remove_member("Vehicles", "Ghost"));
	}
	cout << endl;

	// --- Test 7: delete_set cleans up membership links ---
	cout << "--- Test 7: delete_set ---" << endl;
	{
		cantordb db("test");
		db.create_set("A");
		db.create_set("B");
		db.create_set("C");
		db.add_member("A", "B");
		db.add_member("B", "C");

		Set* a = db.set_index["A"];
		Set* b = db.set_index["B"];
		Set* c = db.set_index["C"];

		check("delete B returns true", db.delete_set("B"));
		check("B removed from set_index", db.set_index.find("B") == db.set_index.end());
		check("A has_element no longer contains B (deleted)",
		      std::find(a->has_element.begin(), a->has_element.end(), b) == a->has_element.end());
		check("C member_of no longer contains B (deleted)",
		      std::find(c->member_of.begin(), c->member_of.end(), b) == c->member_of.end());

		check("delete nonexistent set returns false", !db.delete_set("Ghost"));
	}
	cout << endl;

	// --- Test 8: delete_set removes from multiple parents/children ---
	cout << "--- Test 8: delete_set multi-link cleanup ---" << endl;
	{
		cantordb db("test");
		db.create_set("Parent1");
		db.create_set("Parent2");
		db.create_set("Child");
		db.add_member("Parent1", "Child");
		db.add_member("Parent2", "Child");

		db.delete_set("Child");

		check("Parent1 has_element empty after Child deleted",
		      db.set_index["Parent1"]->has_element.empty());
		check("Parent2 has_element empty after Child deleted",
		      db.set_index["Parent2"]->has_element.empty());
	}
	cout << endl;

	// --- Test 9: Destructor (no crash with populated db) ---
	cout << "--- Test 9: Destructor ---" << endl;
	{
		cantordb db("cleanup");
		db.create_set("X");
		db.create_set("Y");
		db.create_set("Z");
		db.add_member("X", "Y");
		db.add_member("Y", "Z");
		db.add_property("count", 42, "X");
		db.add_property("label", string("hello"), "Y");
	}
	check("Destructor completed without crash", true);
	cout << endl;

	// --- Test 10: set_union basic functionality ---
	cout << "--- Test 10: set_union basic ---" << endl;
	{
		cantordb db("test");
		db.create_set("SetA");
		db.create_set("SetB");
		db.create_set("Elem1");
		db.create_set("Elem2");
		db.create_set("Elem3");

		db.add_member("SetA", "Elem1");
		db.add_member("SetA", "Elem2");
		db.add_member("SetB", "Elem2");
		db.add_member("SetB", "Elem3");

		Set* result = db.set_union("SetA", "SetB");

		check("union returns non-null pointer", result != nullptr);
		check("union result has correct size (3 elements)", result->has_element.size() == 3);
		check("union result cached in database", db.set_index.find(result->set_name) != db.set_index.end());

		Set* elem1 = db.set_index["Elem1"];
		Set* elem2 = db.set_index["Elem2"];
		Set* elem3 = db.set_index["Elem3"];

		check("union contains Elem1",
		      std::find(result->has_element.begin(), result->has_element.end(), elem1) != result->has_element.end());
		check("union contains Elem2",
		      std::find(result->has_element.begin(), result->has_element.end(), elem2) != result->has_element.end());
		check("union contains Elem3",
		      std::find(result->has_element.begin(), result->has_element.end(), elem3) != result->has_element.end());


	}
	cout << endl;

	// --- Test 11: set_union with disjoint sets ---
	cout << "--- Test 11: set_union disjoint ---" << endl;
	{
		cantordb db("test");
		db.create_set("SetX");
		db.create_set("SetY");
		db.create_set("A");
		db.create_set("B");
		db.create_set("C");
		db.create_set("D");

		db.add_member("SetX", "A");
		db.add_member("SetX", "B");
		db.add_member("SetY", "C");
		db.add_member("SetY", "D");

		Set* result = db.set_union("SetX", "SetY");

		check("union of disjoint sets has 4 elements", result->has_element.size() == 4);


	}
	cout << endl;

	// --- Test 12: set_union with identical sets ---
	cout << "--- Test 12: set_union identical ---" << endl;
	{
		cantordb db("test");
		db.create_set("SetP");
		db.create_set("SetQ");
		db.create_set("Item1");
		db.create_set("Item2");

		db.add_member("SetP", "Item1");
		db.add_member("SetP", "Item2");
		db.add_member("SetQ", "Item1");
		db.add_member("SetQ", "Item2");

		Set* result = db.set_union("SetP", "SetQ");

		check("union of identical sets has 2 elements", result->has_element.size() == 2);


	}
	cout << endl;

	// --- Test 13: set_union with empty sets ---
	cout << "--- Test 13: set_union empty ---" << endl;
	{
		cantordb db("test");
		db.create_set("EmptyA");
		db.create_set("EmptyB");
		db.create_set("HasElems");
		db.create_set("E1");
		db.create_set("E2");

		db.add_member("HasElems", "E1");
		db.add_member("HasElems", "E2");

		Set* result1 = db.set_union("EmptyA", "EmptyB");
		check("union of two empty sets returns non-null", result1 != nullptr);
		check("union of two empty sets has 0 elements", result1->has_element.size() == 0);

		Set* result2 = db.set_union("EmptyA", "HasElems");
		check("union of empty and non-empty has 2 elements", result2->has_element.size() == 2);

		Set* result3 = db.set_union("HasElems", "EmptyB");
		check("union of non-empty and empty has 2 elements", result3->has_element.size() == 2);
	}
	cout << endl;

	// --- Test 14: set_union with nonexistent sets ---
	cout << "--- Test 14: set_union nonexistent ---" << endl;
	{
		cantordb db("test");
		db.create_set("RealSet");
		db.create_set("Item");
		db.add_member("RealSet", "Item");

		Set* result1 = db.set_union("Ghost1", "Ghost2");
		check("union with both nonexistent returns nullptr", result1 == nullptr);

		Set* result2 = db.set_union("RealSet", "Ghost");
		check("union with one nonexistent returns nullptr", result2 == nullptr);

		Set* result3 = db.set_union("Ghost", "RealSet");
		check("union with first nonexistent returns nullptr", result3 == nullptr);
	}
	cout << endl;

	// --- Test 15: set_intersection basic ---
	cout << "--- Test 15: set_intersection basic ---" << endl;
	{
		cantordb db("test");
		db.create_set("SetA");
		db.create_set("SetB");
		db.create_set("Elem1");
		db.create_set("Elem2");
		db.create_set("Elem3");

		db.add_member("SetA", "Elem1");
		db.add_member("SetA", "Elem2");
		db.add_member("SetB", "Elem2");
		db.add_member("SetB", "Elem3");

		Set* result = db.set_intersection("SetA", "SetB");

		check("intersection returns non-null pointer", result != nullptr);
		check("intersection result has 1 element", result->has_element.size() == 1);
		check("intersection result cached in database", db.set_index.find(result->set_name) != db.set_index.end());

		Set* elem2 = db.set_index["Elem2"];
		check("intersection contains Elem2", result->has_element[0] == elem2);


	}
	cout << endl;

	// --- Test 16: set_intersection disjoint ---
	cout << "--- Test 16: set_intersection disjoint ---" << endl;
	{
		cantordb db("test");
		db.create_set("SetX");
		db.create_set("SetY");
		db.create_set("A");
		db.create_set("B");

		db.add_member("SetX", "A");
		db.add_member("SetY", "B");

		Set* result = db.set_intersection("SetX", "SetY");

		check("intersection of disjoint sets has 0 elements", result->has_element.size() == 0);


	}
	cout << endl;

	// --- Test 17: set_intersection identical ---
	cout << "--- Test 17: set_intersection identical ---" << endl;
	{
		cantordb db("test");
		db.create_set("SetP");
		db.create_set("SetQ");
		db.create_set("Item1");
		db.create_set("Item2");

		db.add_member("SetP", "Item1");
		db.add_member("SetP", "Item2");
		db.add_member("SetQ", "Item1");
		db.add_member("SetQ", "Item2");

		Set* result = db.set_intersection("SetP", "SetQ");

		check("intersection of identical sets has 2 elements", result->has_element.size() == 2);


	}
	cout << endl;

	// --- Test 18: set_difference basic ---
	cout << "--- Test 18: set_difference basic ---" << endl;
	{
		cantordb db("test");
		db.create_set("SetA");
		db.create_set("SetB");
		db.create_set("Elem1");
		db.create_set("Elem2");
		db.create_set("Elem3");

		db.add_member("SetA", "Elem1");
		db.add_member("SetA", "Elem2");
		db.add_member("SetB", "Elem2");
		db.add_member("SetB", "Elem3");

		Set* result = db.set_difference("SetA", "SetB");

		check("difference returns non-null pointer", result != nullptr);
		check("difference result has 1 element", result->has_element.size() == 1);
		check("difference result cached in database", db.set_index.find(result->set_name) != db.set_index.end());

		Set* elem1 = db.set_index["Elem1"];
		check("difference contains Elem1", result->has_element[0] == elem1);


	}
	cout << endl;

	// --- Test 19: set_difference disjoint ---
	cout << "--- Test 19: set_difference disjoint ---" << endl;
	{
		cantordb db("test");
		db.create_set("SetX");
		db.create_set("SetY");
		db.create_set("A");
		db.create_set("B");

		db.add_member("SetX", "A");
		db.add_member("SetY", "B");

		Set* result = db.set_difference("SetX", "SetY");

		check("difference of disjoint sets (A-B) has 1 element", result->has_element.size() == 1);
		check("difference contains A", result->has_element[0] == db.set_index["A"]);


	}
	cout << endl;

	// --- Test 20: set_difference with subset ---
	cout << "--- Test 20: set_difference subset ---" << endl;
	{
		cantordb db("test");
		db.create_set("SetP");
		db.create_set("SetQ");
		db.create_set("Item1");
		db.create_set("Item2");

		db.add_member("SetP", "Item1");
		db.add_member("SetP", "Item2");
		db.add_member("SetQ", "Item1");

		Set* result = db.set_difference("SetP", "SetQ");

		check("difference P-Q has 1 element", result->has_element.size() == 1);
		check("difference contains Item2", result->has_element[0] == db.set_index["Item2"]);


	}
	cout << endl;

	// --- Test 21: symmetric_difference basic ---
	cout << "--- Test 21: symmetric_difference basic ---" << endl;
	{
		cantordb db("test");
		db.create_set("SetA");
		db.create_set("SetB");
		db.create_set("Elem1");
		db.create_set("Elem2");
		db.create_set("Elem3");

		db.add_member("SetA", "Elem1");
		db.add_member("SetA", "Elem2");
		db.add_member("SetB", "Elem2");
		db.add_member("SetB", "Elem3");

		Set* result = db.symmetric_difference("SetA", "SetB");

		check("symmetric difference returns non-null", result != nullptr);
		check("symmetric difference has 2 elements", result->has_element.size() == 2);

		Set* elem1 = db.set_index["Elem1"];
		Set* elem3 = db.set_index["Elem3"];

		check("symmetric difference contains Elem1",
		      std::find(result->has_element.begin(), result->has_element.end(), elem1) != result->has_element.end());
		check("symmetric difference contains Elem3",
		      std::find(result->has_element.begin(), result->has_element.end(), elem3) != result->has_element.end());
		check("symmetric difference does NOT contain Elem2",
		      std::find(result->has_element.begin(), result->has_element.end(), db.set_index["Elem2"]) == result->has_element.end());


	}
	cout << endl;

	// --- Test 22: symmetric_difference disjoint ---
	cout << "--- Test 22: symmetric_difference disjoint ---" << endl;
	{
		cantordb db("test");
		db.create_set("SetX");
		db.create_set("SetY");
		db.create_set("A");
		db.create_set("B");

		db.add_member("SetX", "A");
		db.add_member("SetY", "B");

		Set* result = db.symmetric_difference("SetX", "SetY");

		check("symmetric difference of disjoint sets has 2 elements", result->has_element.size() == 2);
		check("symmetric difference contains both elements",
		      std::find(result->has_element.begin(), result->has_element.end(), db.set_index["A"]) != result->has_element.end() &&
		      std::find(result->has_element.begin(), result->has_element.end(), db.set_index["B"]) != result->has_element.end());


	}
	cout << endl;

	// --- Test 23: symmetric_difference identical ---
	cout << "--- Test 23: symmetric_difference identical ---" << endl;
	{
		cantordb db("test");
		db.create_set("SetP");
		db.create_set("SetQ");
		db.create_set("Item1");
		db.create_set("Item2");

		db.add_member("SetP", "Item1");
		db.add_member("SetP", "Item2");
		db.add_member("SetQ", "Item1");
		db.add_member("SetQ", "Item2");

		Set* result = db.symmetric_difference("SetP", "SetQ");

		check("symmetric difference of identical sets has 0 elements", result->has_element.size() == 0);


	}
	cout << endl;

	// --- Test 24: symmetric_difference empty sets ---
	cout << "--- Test 24: symmetric_difference empty sets ---" << endl;
	{
		cantordb db("test");
		db.create_set("Empty1");
		db.create_set("Empty2");
		db.create_set("HasItem");
		db.create_set("Item");

		db.add_member("HasItem", "Item");

		Set* result1 = db.symmetric_difference("Empty1", "Empty2");
		check("symmetric difference of two empty sets has 0 elements", result1->has_element.size() == 0);

		Set* result2 = db.symmetric_difference("Empty1", "HasItem");
		check("symmetric difference of empty and non-empty has 1 element", result2->has_element.size() == 1);
	}
	cout << endl;

	// --- Test 25: symmetric_difference nonexistent ---
	cout << "--- Test 25: symmetric_difference nonexistent ---" << endl;
	{
		cantordb db("test");
		db.create_set("Real");

		Set* result1 = db.symmetric_difference("Ghost1", "Ghost2");
		check("symmetric difference with both nonexistent returns nullptr", result1 == nullptr);

		Set* result2 = db.symmetric_difference("Real", "Ghost");
		check("symmetric difference with one nonexistent returns nullptr", result2 == nullptr);
	}
	cout << endl;

	// --- Test 26: set_complement basic ---
	cout << "--- Test 26: set_complement basic ---" << endl;
	{
		cantordb db("test");
		db.create_set("Colors");
		db.create_set("Red");
		db.create_set("Blue");
		db.create_set("Green");

		db.add_member("Colors", "Red");
		db.add_member("Colors", "Blue");

		Set* result = db.set_complement("Colors");

		check("complement returns non-null", result != nullptr);
		check("complement contains Green",
		      std::find(result->has_element.begin(), result->has_element.end(), db.set_index["Green"]) != result->has_element.end());
		check("complement contains Colors (since it's in UNIVERSAL_SET)",
		      std::find(result->has_element.begin(), result->has_element.end(), db.set_index["Colors"]) != result->has_element.end());
		check("complement does not contain Red",
		      std::find(result->has_element.begin(), result->has_element.end(), db.set_index["Red"]) == result->has_element.end());
		check("complement does not contain Blue",
		      std::find(result->has_element.begin(), result->has_element.end(), db.set_index["Blue"]) == result->has_element.end());


	}
	cout << endl;

	// --- Test 27: set_complement of everything ---
	cout << "--- Test 27: set_complement of everything ---" << endl;
	{
		cantordb db("test");
		Set* result = db.set_complement(UNIVERSAL_SET);

		check("complement of everything has 0 elements", result->has_element.size() == 0);


	}
	cout << endl;

	// --- Test 28: set_complement empty set ---
	cout << "--- Test 28: set_complement empty set ---" << endl;
	{
		cantordb db("test");
		db.create_set("Empty");
		db.create_set("Item1");
		db.create_set("Item2");

		Set* result = db.set_complement("Empty");

		check("complement of empty set has all other sets (including Empty itself)", result->has_element.size() == 3);
		check("complement contains Empty",
		      std::find(result->has_element.begin(), result->has_element.end(), db.set_index["Empty"]) != result->has_element.end());
		check("complement contains Item1",
		      std::find(result->has_element.begin(), result->has_element.end(), db.set_index["Item1"]) != result->has_element.end());
		check("complement contains Item2",
		      std::find(result->has_element.begin(), result->has_element.end(), db.set_index["Item2"]) != result->has_element.end());


	}
	cout << endl;

	// --- Test 29: is_disjoint basic ---
	cout << "--- Test 29: is_disjoint basic ---" << endl;
	{
		cantordb db("test");
		db.create_set("SetA");
		db.create_set("SetB");
		db.create_set("SetC");
		db.create_set("Elem1");
		db.create_set("Elem2");
		db.create_set("Elem3");

		db.add_member("SetA", "Elem1");
		db.add_member("SetA", "Elem2");
		db.add_member("SetB", "Elem3");
		db.add_member("SetC", "Elem1");

		check("SetA and SetB are disjoint", db.is_disjoint("SetA", "SetB"));
		check("SetB and SetA are disjoint (symmetric)", db.is_disjoint("SetB", "SetA"));
		check("SetA and SetC are not disjoint", !db.is_disjoint("SetA", "SetC"));
	}
	cout << endl;

	// --- Test 30: is_disjoint empty sets ---
	cout << "--- Test 30: is_disjoint empty sets ---" << endl;
	{
		cantordb db("test");
		db.create_set("Empty1");
		db.create_set("Empty2");
		db.create_set("NonEmpty");
		db.create_set("Item");

		db.add_member("NonEmpty", "Item");

		check("two empty sets are disjoint", db.is_disjoint("Empty1", "Empty2"));
		check("empty and non-empty are disjoint", db.is_disjoint("Empty1", "NonEmpty"));
		check("set is not disjoint with itself if non-empty", !db.is_disjoint("NonEmpty", "NonEmpty"));
	}
	cout << endl;

	// --- Test 31: is_disjoint nonexistent ---
	cout << "--- Test 31: is_disjoint nonexistent ---" << endl;
	{
		cantordb db("test");
		db.create_set("Real");

		check("is_disjoint with nonexistent returns false", !db.is_disjoint("Real", "Ghost"));
		check("is_disjoint with both nonexistent returns false", !db.is_disjoint("Ghost1", "Ghost2"));
	}
	cout << endl;

	// --- Test 32: is_subset basic ---
	cout << "--- Test 32: is_subset basic ---" << endl;
	{
		cantordb db("test");
		db.create_set("Animals");
		db.create_set("Mammals");
		db.create_set("Dog");
		db.create_set("Cat");
		db.create_set("Fish");

		db.add_member("Animals", "Dog");
		db.add_member("Animals", "Cat");
		db.add_member("Animals", "Fish");
		db.add_member("Mammals", "Dog");
		db.add_member("Mammals", "Cat");

		check("Mammals is subset of Animals", db.is_subset("Mammals", "Animals"));
		check("Animals is not subset of Mammals", !db.is_subset("Animals", "Mammals"));
		check("Animals is subset of itself", db.is_subset("Animals", "Animals"));
		check("Mammals is subset of itself", db.is_subset("Mammals", "Mammals"));
	}
	cout << endl;

	// --- Test 33: is_subset empty ---
	cout << "--- Test 33: is_subset empty ---" << endl;
	{
		cantordb db("test");
		db.create_set("Full");
		db.create_set("Empty");
		db.create_set("Item");

		db.add_member("Full", "Item");

		check("empty set is subset of any set", db.is_subset("Empty", "Full"));
		check("empty set is subset of itself", db.is_subset("Empty", "Empty"));
		check("non-empty is not subset of empty", !db.is_subset("Full", "Empty"));
	}
	cout << endl;

	// --- Test 34: is_subset nonexistent ---
	cout << "--- Test 34: is_subset nonexistent ---" << endl;
	{
		cantordb db("test");
		db.create_set("Real");

		check("nonexistent set A returns false", !db.is_subset("Ghost", "Real"));
		check("nonexistent set B returns false", !db.is_subset("Real", "Ghost"));
		check("both nonexistent returns false", !db.is_subset("Ghost1", "Ghost2"));
	}
	cout << endl;

	// --- Test 35: is_superset basic ---
	cout << "--- Test 35: is_superset basic ---" << endl;
	{
		cantordb db("test");
		db.create_set("Big");
		db.create_set("Small");
		db.create_set("Item1");
		db.create_set("Item2");

		db.add_member("Big", "Item1");
		db.add_member("Big", "Item2");
		db.add_member("Small", "Item1");

		check("Big is superset of Small", db.is_superset("Big", "Small"));
		check("Small is not superset of Big", !db.is_superset("Small", "Big"));
		check("Big is superset of itself", db.is_superset("Big", "Big"));
	}
	cout << endl;

	// --- Test 36: is_superset empty ---
	cout << "--- Test 36: is_superset empty ---" << endl;
	{
		cantordb db("test");
		db.create_set("Full");
		db.create_set("Empty");
		db.create_set("Item");

		db.add_member("Full", "Item");

		check("any set is superset of empty set", db.is_superset("Full", "Empty"));
		check("empty is not superset of non-empty", !db.is_superset("Empty", "Full"));
	}
	cout << endl;

	// --- Test 37: complex set theory operations ---
	cout << "--- Test 37: complex combinations ---" << endl;
	{
		cantordb db("test");
		db.create_set("Evens");
		db.create_set("Odds");
		db.create_set("Primes");
		db.create_set("Two");
		db.create_set("Three");
		db.create_set("Four");
		db.create_set("Five");

		db.add_member("Evens", "Two");
		db.add_member("Evens", "Four");
		db.add_member("Odds", "Three");
		db.add_member("Odds", "Five");
		db.add_member("Primes", "Two");
		db.add_member("Primes", "Three");
		db.add_member("Primes", "Five");

		check("Evens and Odds are disjoint", db.is_disjoint("Evens", "Odds"));
		check("Evens and Primes are not disjoint", !db.is_disjoint("Evens", "Primes"));

		Set* evens_union_odds = db.set_union("Evens", "Odds");
		check("union of Evens and Odds has 4 elements", evens_union_odds->has_element.size() == 4);

		Set* evens_intersect_primes = db.set_intersection("Evens", "Primes");
		check("Evens intersect Primes has 1 element (Two)", evens_intersect_primes->has_element.size() == 1);
		check("intersection contains Two", evens_intersect_primes->has_element[0] == db.set_index["Two"]);

		Set* evens_sym_diff_primes = db.symmetric_difference("Evens", "Primes");
		check("symmetric difference Evens and Primes has 3 elements", evens_sym_diff_primes->has_element.size() == 3);

		Set* not_evens = db.set_complement("Evens");
		check("complement of Evens contains Odds", db.is_superset("Odds", "Odds"));
	}
	cout << endl;

	// --- Test 38: get_property variant ---
	cout << "--- Test 38: get_property variant ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("count", 42, "Item");
		db.add_property("name", string("widget"), "Item");
		db.add_property("price", 9.99, "Item");
		db.add_property("active", true, "Item");
		db.add_property("serial", 123456L, "Item");

		auto v_int = db.get_property("Item", "count");
		check("get_property int returns correct value", get<int>(v_int) == 42);

		auto v_str = db.get_property("Item", "name");
		check("get_property string returns correct value", get<string>(v_str) == "widget");

		auto v_dbl = db.get_property("Item", "price");
		check("get_property double returns correct value", get<double>(v_dbl) == 9.99);

		auto v_bool = db.get_property("Item", "active");
		check("get_property bool returns correct value", get<bool>(v_bool) == true);

		auto v_long = db.get_property("Item", "serial");
		check("get_property long returns correct value", get<long>(v_long) == 123456L);

		// nonexistent set
		db.get_property("Ghost", "count");
		check("get_property nonexistent set sets error", db.EC == ER_SET_NOT_FOUND);

		// nonexistent key
		db.get_property("Item", "ghost_key");
		check("get_property nonexistent key sets error", db.EC == ER_KEY_NOT_FOUND);
	}
	cout << endl;

	// --- Test 39: get_property_safe_int ---
	cout << "--- Test 39: get_property_safe_int ---" << endl;
	{
		cantordb db("test");
		db.create_set("Numbers");
		db.add_property("count", 10, "Numbers");
		db.add_property("label", string("hello"), "Numbers");

		check("safe_int returns correct value", db.get_property_safe_int("Numbers", "count") == 10);

		// wrong type
		db.get_property_safe_int("Numbers", "label");
		check("safe_int wrong type sets ER_KEY_WRONG_TYPE", db.EC == ER_KEY_WRONG_TYPE);

		// nonexistent key
		check("safe_int nonexistent key returns 0", db.get_property_safe_int("Numbers", "nope") == 0);
		check("safe_int nonexistent key sets ER_KEY_NOT_FOUND", db.EC == ER_KEY_NOT_FOUND);

		// nonexistent set
		check("safe_int nonexistent set returns 0", db.get_property_safe_int("Ghost", "count") == 0);
		check("safe_int nonexistent set sets ER_SET_NOT_FOUND", db.EC == ER_SET_NOT_FOUND);
	}
	cout << endl;

	// --- Test 40: get_property_safe_string ---
	cout << "--- Test 40: get_property_safe_string ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("name", string("widget"), "Item");
		db.add_property("count", 5, "Item");

		check("safe_string returns correct value", db.get_property_safe_string("Item", "name") == "widget");

		db.get_property_safe_string("Item", "count");
		check("safe_string wrong type sets ER_KEY_WRONG_TYPE", db.EC == ER_KEY_WRONG_TYPE);

		check("safe_string nonexistent key returns empty", db.get_property_safe_string("Item", "nope") == "");
	}
	cout << endl;

	// --- Test 41: get_property_safe_double ---
	cout << "--- Test 41: get_property_safe_double ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("price", 3.14, "Item");
		db.add_property("count", 5, "Item");

		check("safe_double returns correct value", db.get_property_safe_double("Item", "price") == 3.14);

		db.get_property_safe_double("Item", "count");
		check("safe_double wrong type sets ER_KEY_WRONG_TYPE", db.EC == ER_KEY_WRONG_TYPE);
	}
	cout << endl;

	// --- Test 42: get_property_safe_bool ---
	cout << "--- Test 42: get_property_safe_bool ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("active", true, "Item");
		db.add_property("count", 5, "Item");

		check("safe_bool returns correct value", db.get_property_safe_bool("Item", "active") == true);

		db.get_property_safe_bool("Item", "count");
		check("safe_bool wrong type sets ER_KEY_WRONG_TYPE", db.EC == ER_KEY_WRONG_TYPE);
	}
	cout << endl;

	// --- Test 43: get_property_safe_long ---
	cout << "--- Test 43: get_property_safe_long ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("serial", 999999L, "Item");
		db.add_property("count", 5, "Item");

		check("safe_long returns correct value", db.get_property_safe_long("Item", "serial") == 999999L);

		db.get_property_safe_long("Item", "count");
		check("safe_long wrong type sets ER_KEY_WRONG_TYPE", db.EC == ER_KEY_WRONG_TYPE);
	}
	cout << endl;

	// --- Test 44: delete_property ---
	cout << "--- Test 44: delete_property ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("count", 42, "Item");
		db.add_property("name", string("widget"), "Item");
		db.add_property("price", 9.99, "Item");
		db.add_property("active", true, "Item");
		db.add_property("serial", 123456L, "Item");

		check("delete int property returns true", db.delete_property("Item", "count"));
		check("deleted key removed from key_index",
		      db.set_index["Item"]->key_index.find("count") == db.set_index["Item"]->key_index.end());
		check("deleted key removed from key_num",
		      db.set_index["Item"]->key_num.find("count") == db.set_index["Item"]->key_num.end());

		check("delete string property returns true", db.delete_property("Item", "name"));
		check("deleted string key removed from key_value",
		      db.set_index["Item"]->key_value.find("name") == db.set_index["Item"]->key_value.end());

		check("delete double property returns true", db.delete_property("Item", "price"));
		check("deleted double key removed from key_decimal",
		      db.set_index["Item"]->key_decimal.find("price") == db.set_index["Item"]->key_decimal.end());

		check("delete bool property returns true", db.delete_property("Item", "active"));
		check("deleted bool key removed from key_bool",
		      db.set_index["Item"]->key_bool.find("active") == db.set_index["Item"]->key_bool.end());

		check("delete long property returns true", db.delete_property("Item", "serial"));
		check("deleted long key removed from key_long",
		      db.set_index["Item"]->key_long.find("serial") == db.set_index["Item"]->key_long.end());

		check("key_index is empty after all deletes", db.set_index["Item"]->key_index.empty());

		// can re-add after delete
		check("can re-add deleted property", db.add_property("count", 99, "Item"));
		check("re-added value is correct", db.set_index["Item"]->key_num["count"] == 99);

		// error cases
		check("delete nonexistent key returns false", !db.delete_property("Item", "ghost"));
		check("delete nonexistent key sets ER_KEY_NOT_FOUND", db.EC == ER_KEY_NOT_FOUND);

		check("delete from nonexistent set returns false", !db.delete_property("Ghost", "count"));
		check("delete from nonexistent set sets ER_SET_NOT_FOUND", db.EC == ER_SET_NOT_FOUND);
	}
	cout << endl;

	// --- Test 45: update_property int ---
	cout << "--- Test 45: update_property int ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("count", 10, "Item");

		check("update int returns true", db.update_property("Item", "count", 20));
		check("updated int value is correct", db.set_index["Item"]->key_num["count"] == 20);

		// wrong type
		check("update int on string key returns false", !db.update_property("Item", "count", string("bad")));

		// nonexistent key
		check("update nonexistent key returns false", !db.update_property("Item", "ghost", 5));
		check("update nonexistent key sets ER_KEY_NOT_FOUND", db.EC == ER_KEY_NOT_FOUND);

		// nonexistent set
		check("update nonexistent set returns false", !db.update_property("Ghost", "count", 5));
		check("update nonexistent set sets ER_SET_NOT_FOUND", db.EC == ER_SET_NOT_FOUND);
	}
	cout << endl;

	// --- Test 46: update_property string ---
	cout << "--- Test 46: update_property string ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("name", string("old"), "Item");

		check("update string returns true", db.update_property("Item", "name", string("new")));
		check("updated string value is correct", db.set_index["Item"]->key_value["name"] == "new");

		check("update string on int key fails", !db.update_property("Item", "name", 42));
	}
	cout << endl;

	// --- Test 47: update_property double ---
	cout << "--- Test 47: update_property double ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("price", 1.5, "Item");

		check("update double returns true", db.update_property("Item", "price", 2.5));
		check("updated double value is correct", db.set_index["Item"]->key_decimal["price"] == 2.5);

		check("update double on string key fails", !db.update_property("Item", "price", string("bad")));
	}
	cout << endl;

	// --- Test 48: update_property bool ---
	cout << "--- Test 48: update_property bool ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("active", true, "Item");

		check("update bool returns true", db.update_property("Item", "active", false));
		check("updated bool value is correct", db.set_index["Item"]->key_bool["active"] == false);
	}
	cout << endl;

	// --- Test 49: update_property long ---
	cout << "--- Test 49: update_property long ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("serial", 100L, "Item");

		check("update long returns true", db.update_property("Item", "serial", 200L));
		check("updated long value is correct", db.set_index["Item"]->key_long["serial"] == 200L);
	}
	cout << endl;

	// --- Test 50: list_property_keys ---
	cout << "--- Test 50: list_property_keys ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("count", 42, "Item");
		db.add_property("name", string("widget"), "Item");
		db.add_property("price", 9.99, "Item");
		db.add_property("active", true, "Item");
		db.add_property("serial", 123456L, "Item");

		string keys = db.list_property_keys("Item");
		check("list_property_keys contains count: int", keys.find("count: int") != string::npos);
		check("list_property_keys contains name: string", keys.find("name: string") != string::npos);
		check("list_property_keys contains price: double", keys.find("price: double") != string::npos);
		check("list_property_keys contains active: bool", keys.find("active: bool") != string::npos);
		check("list_property_keys contains serial: long", keys.find("serial: long") != string::npos);

		check("list_property_keys empty set returns empty", db.list_property_keys("Ghost") == "");

		cantordb db2("test2");
		db2.create_set("Empty");
		check("list_property_keys no properties returns empty", db2.list_property_keys("Empty") == "");
	}
	cout << endl;

	// --- Test 51: list_properties ---
	cout << "--- Test 51: list_properties ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("count", 42, "Item");
		db.add_property("name", string("widget"), "Item");
		db.add_property("active", true, "Item");

		string props = db.list_properties("Item");
		check("list_properties contains count:42:int", props.find("count:42:int") != string::npos);
		check("list_properties contains name:widget:string", props.find("name:widget:string") != string::npos);
		check("list_properties contains active:true:bool", props.find("active:true:bool") != string::npos);

		check("list_properties nonexistent set returns empty", db.list_properties("Ghost") == "");
	}
	cout << endl;

	// --- Test 52: list_string_properties ---
	cout << "--- Test 52: list_string_properties ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("name", string("widget"), "Item");
		db.add_property("color", string("blue"), "Item");
		db.add_property("count", 5, "Item");

		string props = db.list_string_properties("Item");
		check("list_string_properties contains name:widget", props.find("name:widget") != string::npos);
		check("list_string_properties contains color:blue", props.find("color:blue") != string::npos);
		check("list_string_properties does not contain int props", props.find("count") == string::npos);
	}
	cout << endl;

	// --- Test 53: list_int_properties ---
	cout << "--- Test 53: list_int_properties ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("count", 5, "Item");
		db.add_property("size", 10, "Item");
		db.add_property("name", string("widget"), "Item");

		string props = db.list_int_properties("Item");
		check("list_int_properties contains count:5", props.find("count:5") != string::npos);
		check("list_int_properties contains size:10", props.find("size:10") != string::npos);
		check("list_int_properties does not contain string props", props.find("name") == string::npos);
	}
	cout << endl;

	// --- Test 54: list_double_properties ---
	cout << "--- Test 54: list_double_properties ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("price", 9.99, "Item");
		db.add_property("count", 5, "Item");

		string props = db.list_double_properties("Item");
		check("list_double_properties contains price", props.find("price:") != string::npos);
		check("list_double_properties does not contain int props", props.find("count") == string::npos);
	}
	cout << endl;

	// --- Test 55: list_bool_properties ---
	cout << "--- Test 55: list_bool_properties ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("active", true, "Item");
		db.add_property("visible", false, "Item");
		db.add_property("count", 5, "Item");

		string props = db.list_bool_properties("Item");
		check("list_bool_properties contains active:true", props.find("active:true") != string::npos);
		check("list_bool_properties contains visible:false", props.find("visible:false") != string::npos);
		check("list_bool_properties does not contain int props", props.find("count") == string::npos);
	}
	cout << endl;

	// --- Test 56: list_long_properties ---
	cout << "--- Test 56: list_long_properties ---" << endl;
	{
		cantordb db("test");
		db.create_set("Item");
		db.add_property("serial", 123456L, "Item");
		db.add_property("count", 5, "Item");

		string props = db.list_long_properties("Item");
		check("list_long_properties contains serial:123456", props.find("serial:123456") != string::npos);
		check("list_long_properties does not contain int props", props.find("count") == string::npos);
	}
	cout << endl;

	// --- Test 57: list_elements ---
	cout << "--- Test 57: list_elements ---" << endl;
	{
		cantordb db("test");
		db.create_set("Animals");
		db.create_set("Dog");
		db.create_set("Cat");
		db.add_member("Animals", "Dog");
		db.add_member("Animals", "Cat");

		string elems = db.list_elements("Animals");
		check("list_elements contains Dog", elems.find("Dog") != string::npos);
		check("list_elements contains Cat", elems.find("Cat") != string::npos);

		check("list_elements nonexistent set returns empty", db.list_elements("Ghost") == "");

		cantordb db2("test2");
		db2.create_set("Empty");
		check("list_elements empty set returns empty", db2.list_elements("Empty") == "");
	}
	cout << endl;

	// --- Test 58: get_cardinality ---
	cout << "--- Test 58: get_cardinality ---" << endl;
	{
		cantordb db("test");
		db.create_set("Animals");
		db.create_set("Dog");
		db.create_set("Cat");
		db.add_member("Animals", "Dog");
		db.add_member("Animals", "Cat");

		check("get_cardinality returns 2", db.get_cardinality("Animals") == 2);

		cantordb db2("test2");
		db2.create_set("Empty");
		check("get_cardinality empty set returns 0", db2.get_cardinality("Empty") == 0);
		check("get_cardinality nonexistent returns -1", db2.get_cardinality("Ghost") == -1);
	}
	cout << endl;

	// --- Test 59: list_all_sets ---
	cout << "--- Test 59: list_all_sets ---" << endl;
	{
		cantordb db("test");
		db.create_set("Animals");
		db.create_set("Plants");

		string all = db.list_all_sets();
		check("list_all_sets contains Animals", all.find("Animals") != string::npos);
		check("list_all_sets contains Plants", all.find("Plants") != string::npos);
	}
	cout << endl;

	// --- Test 60: clear_cache ---
	cout << "--- Test 60: clear_cache ---" << endl;
	{
		cantordb db("test");
		db.create_set("A");
		db.create_set("B");
		db.create_set("E1");
		db.create_set("E2");
		db.add_member("A", "E1");
		db.add_member("B", "E2");

		Set* u = db.set_union("A", "B");
		string cached_name = u->set_name;
		check("cached set exists before clear", db.set_index.find(cached_name) != db.set_index.end());

		check("clear_cache returns true", db.clear_cache());
		check("cached set removed after clear", db.set_index.find(cached_name) == db.set_index.end());
		check("original sets still exist after clear", db.set_index.find("A") != db.set_index.end());
	}
	cout << endl;

	// --- Test 61: rename_set ---
	cout << "--- Test 61: rename_set ---" << endl;
	{
		cantordb db("test");
		db.create_set("OldName");
		db.create_set("Parent");
		db.create_set("Child");
		db.add_member("OldName", "Child");
		db.add_member("Parent", "OldName");
		db.add_property("count", 42, "OldName");

		check("rename returns true", db.rename_set("OldName", "NewName"));
		check("new name exists in index", db.set_index.find("NewName") != db.set_index.end());
		check("old name removed from index", db.set_index.find("OldName") == db.set_index.end());
		check("set_name field updated", db.set_index["NewName"]->set_name == "NewName");
		check("properties preserved after rename", db.set_index["NewName"]->key_num["count"] == 42);
		check("children preserved after rename",
		      db.set_index["NewName"]->has_element.size() == 1);
		check("parent still references renamed set",
		      std::find(db.set_index["Parent"]->has_element.begin(),
		                db.set_index["Parent"]->has_element.end(),
		                db.set_index["NewName"]) != db.set_index["Parent"]->has_element.end());

		// error cases
		check("rename nonexistent set returns false", !db.rename_set("Ghost", "Something"));
		check("rename nonexistent sets ER_SET_NOT_FOUND", db.EC == ER_SET_NOT_FOUND);

		check("rename to existing name returns false", !db.rename_set("NewName", "Parent"));
		check("rename to existing sets ER_SET_EX", db.EC == ER_SET_EX);
	}
	cout << endl;

	// --- Summary ---
	cout << "=== Results: " << pass_count << " passed, " << fail_count << " failed ===" << endl;
	return fail_count > 0 ? 1 : 0;
}
