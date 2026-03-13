#include "cantordb.h"
#include "parser.h"
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
	cout << "=== Parser Tests ===" << endl << endl;

	cantordb db("test");
	db.create_set("Animals");
	db.create_set("Dog");
	db.create_set("Cat");
	db.create_set("Fish");
	db.add_member("Animals", "Dog");
	db.add_member("Animals", "Cat");
	db.add_member("Animals", "Fish");

	// --- Test 1: GET ELEMENTS OF ---
	cout << "--- Test 1: GET ELEMENTS OF ---" << endl;
	{
		string result = parse_query(db, "GET ELEMENTS OF Animals");
		check("returns Dog", result.find("Dog") != string::npos);
		check("returns Cat", result.find("Cat") != string::npos);
		check("returns Fish", result.find("Fish") != string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 2: case insensitive ---
	cout << "--- Test 2: case insensitive ---" << endl;
	{
		string result = parse_query(db, "get elements of Animals");
		check("lowercase get works", result.find("Dog") != string::npos);
	}
	cout << endl;

	// --- Test 3: GET SETS OF ---
	cout << "--- Test 3: GET SETS OF ---" << endl;
	{
		string result = parse_query(db, "GET SETS OF Dog");
		check("returns Animals", result.find("Animals") != string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 3b: GET SETS (no OF) returns error ---
	cout << "--- Test 3b: GET SETS (no OF) returns error ---" << endl;
	{
		string result = parse_query(db, "GET SETS");
		check("returns error", result.find("Syntax Error") != string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 4: missing GET ---
	cout << "--- Test 4: error - missing GET ---" << endl;
	{
		string result = parse_query(db, "ELEMENTS OF Animals");
		check("returns error message", result.find("Syntax Error") != string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 5: missing query type ---
	cout << "--- Test 5: error - missing query type ---" << endl;
	{
		string result = parse_query(db, "GET OF Animals");
		check("returns error message", result.find("Syntax Error") != string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 6: missing OF ---
	cout << "--- Test 6: error - missing OF ---" << endl;
	{
		string result = parse_query(db, "GET ELEMENTS Animals");
		check("returns error message", result.find("Syntax Error") != string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 7: nonexistent set ---
	cout << "--- Test 7: nonexistent set ---" << endl;
	{
		string result = parse_query(db, "GET ELEMENTS OF Ghost");
		check("returns empty for nonexistent set", result == "");
		cout << "  Result: [" << result << "]" << endl;
	}
	cout << endl;

	// --- Test 8: WHERE greater than ---
	cout << "--- Test 8: WHERE greater than ---" << endl;
	{
		db.add_property("legs", 4, "Dog");
		db.add_property("legs", 4, "Cat");
		db.add_property("legs", 0, "Fish");

		string result = parse_query(db, "GET ELEMENTS OF Animals WHERE legs > 0");
		check("WHERE > returns Dog", result.find("Dog") != string::npos);
		check("WHERE > returns Cat", result.find("Cat") != string::npos);
		check("WHERE > excludes Fish", result.find("Fish") == string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 9: WHERE equals ---
	cout << "--- Test 9: WHERE equals ---" << endl;
	{
		string result = parse_query(db, "GET ELEMENTS OF Animals WHERE legs = 0");
		check("WHERE = returns Fish", result.find("Fish") != string::npos);
		check("WHERE = excludes Dog", result.find("Dog") == string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 10: WHERE less than ---
	cout << "--- Test 10: WHERE less than ---" << endl;
	{
		string result = parse_query(db, "GET ELEMENTS OF Animals WHERE legs < 4");
		check("WHERE < returns Fish", result.find("Fish") != string::npos);
		check("WHERE < excludes Dog", result.find("Dog") == string::npos);
		check("WHERE < excludes Cat", result.find("Cat") == string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 11: WHERE greater or equal ---
	cout << "--- Test 11: WHERE greater or equal ---" << endl;
	{
		string result = parse_query(db, "GET ELEMENTS OF Animals WHERE legs >= 4");
		check("WHERE >= returns Dog", result.find("Dog") != string::npos);
		check("WHERE >= returns Cat", result.find("Cat") != string::npos);
		check("WHERE >= excludes Fish", result.find("Fish") == string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 12: WHERE less or equal ---
	cout << "--- Test 12: WHERE less or equal ---" << endl;
	{
		string result = parse_query(db, "GET ELEMENTS OF Animals WHERE legs <= 0");
		check("WHERE <= returns Fish", result.find("Fish") != string::npos);
		check("WHERE <= excludes Dog", result.find("Dog") == string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 13: WHERE string equals ---
	cout << "--- Test 13: WHERE string equals ---" << endl;
	{
		db.add_property("sound", string("bark"), "Dog");
		db.add_property("sound", string("meow"), "Cat");
		db.add_property("sound", string("blub"), "Fish");

		string result = parse_query(db, "GET ELEMENTS OF Animals WHERE sound = bark");
		check("WHERE string = returns Dog", result.find("Dog") != string::npos);
		check("WHERE string = excludes Cat", result.find("Cat") == string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 14: WHERE bool equals ---
	cout << "--- Test 14: WHERE bool equals ---" << endl;
	{
		db.add_property("domestic", true, "Dog");
		db.add_property("domestic", true, "Cat");
		db.add_property("domestic", false, "Fish");

		string result = parse_query(db, "GET ELEMENTS OF Animals WHERE domestic = true");
		check("WHERE bool returns Dog", result.find("Dog") != string::npos);
		check("WHERE bool returns Cat", result.find("Cat") != string::npos);
		check("WHERE bool excludes Fish", result.find("Fish") == string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 15: WHERE error - no property ---
	cout << "--- Test 15: WHERE error - no property ---" << endl;
	{
		string result = parse_query(db, "GET ELEMENTS OF Animals WHERE");
		check("missing property returns error", result.find("Syntax Error") != string::npos || result.find("Error") != string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 16: WHERE with set algebra ---
	cout << "--- Test 16: WHERE with set algebra ---" << endl;
	{
		db.create_set("Pets");
		db.create_set("Hamster");
		db.add_property("legs", 4, "Hamster");
		db.add_member("Pets", "Dog");
		db.add_member("Pets", "Cat");
		db.add_member("Pets", "Hamster");

		string result = parse_query(db, "GET ELEMENTS OF Animals WHERE legs > 0 UNION Pets WHERE legs > 0");
		check("set algebra + WHERE returns Dog", result.find("Dog") != string::npos);
		check("set algebra + WHERE returns Cat", result.find("Cat") != string::npos);
		check("set algebra + WHERE returns Hamster", result.find("Hamster") != string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 17: WHERE double property with integer literal ---
	cout << "--- Test 17: WHERE double property with integer literal ---" << endl;
	{
		db.create_set("Metals");
		db.create_set("Lithium");
		db.create_set("Iron");
		db.create_set("Copernicium");
		db.add_property("atomic_mass", 6.94, "Lithium");
		db.add_property("atomic_mass", 55.85, "Iron");
		db.add_property("atomic_mass", 285.0, "Copernicium");
		db.add_member("Metals", "Lithium");
		db.add_member("Metals", "Iron");
		db.add_member("Metals", "Copernicium");

		string result = parse_query(db, "GET ELEMENTS OF Metals WHERE atomic_mass < 30");
		check("WHERE double < int literal returns Lithium", result.find("Lithium") != string::npos);
		check("WHERE double < int literal excludes Iron", result.find("Iron") == string::npos);
		check("WHERE double < int literal excludes Copernicium", result.find("Copernicium") == string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 18: CREATE SETS ---
	cout << "--- Test 17: CREATE SETS ---" << endl;
	{
		string result = parse_query(db, "CREATE SETS Bird Lizard");
		check("CREATE returns success", result.find("Created") != string::npos);
		check("Bird exists", db.set_index.find("Bird") != db.set_index.end());
		check("Lizard exists", db.set_index.find("Lizard") != db.set_index.end());
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 19: TRASH SETS ---
	cout << "--- Test 19: TRASH SETS ---" << endl;
	{
		string result = parse_query(db, "TRASH SETS Lizard");
		check("TRASH returns success", result.find("Trashed") != string::npos);
		check("Lizard removed from index", db.set_index.find("Lizard") == db.set_index.end());
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 20: ADD element TO set ---
	cout << "--- Test 20: ADD element TO set ---" << endl;
	{
		db.create_set("Reptiles");
		db.create_set("Snake");
		string result = parse_query(db, "ADD Snake TO Reptiles");
		check("ADD returns success", result.find("Added") != string::npos);
		check("Snake is element of Reptiles", db.is_element("Snake", "Reptiles"));
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 21: REMOVE element FROM set ---
	cout << "--- Test 21: REMOVE element FROM set ---" << endl;
	{
		string result = parse_query(db, "REMOVE Snake FROM Reptiles");
		check("REMOVE returns success", result.find("Removed") != string::npos);
		check("Snake no longer element", !db.is_element("Snake", "Reptiles"));
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 22: ADD PROPERTY with value TO set ---
	cout << "--- Test 22: ADD PROPERTY with value ---" << endl;
	{
		string result = parse_query(db, "ADD PROPERTY speed = 5 TO Snake");
		check("ADD PROPERTY returns success", result.find("Added property") != string::npos);
		check("speed value is 5", db.get_property_safe_int("Snake", "speed") == 5);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 23: ADD PROPERTY without value (zero-init) ---
	cout << "--- Test 23: ADD PROPERTY zero-init ---" << endl;
	{
		string result = parse_query(db, "ADD PROPERTY venom TO Snake");
		check("ADD PROPERTY returns success", result.find("Added property") != string::npos);
		check("venom value is 0", db.get_property_safe_int("Snake", "venom") == 0);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 24: ADD PROPERTY string value ---
	cout << "--- Test 24: ADD PROPERTY string value ---" << endl;
	{
		string result = parse_query(db, "ADD PROPERTY color = green TO Snake");
		check("ADD PROPERTY returns success", result.find("Added property") != string::npos);
		check("color value is green", db.get_property_safe_string("Snake", "color") == "green");
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 25: ADD PROPERTY bool value ---
	cout << "--- Test 25: ADD PROPERTY bool value ---" << endl;
	{
		string result = parse_query(db, "ADD PROPERTY venomous = true TO Snake");
		check("ADD PROPERTY returns success", result.find("Added property") != string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 26: REMOVE PROPERTY FROM set ---
	cout << "--- Test 26: REMOVE PROPERTY FROM set ---" << endl;
	{
		string result = parse_query(db, "REMOVE PROPERTY speed FROM Snake");
		check("REMOVE PROPERTY returns success", result.find("Removed property") != string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 27: DELETE SETS (hard delete) ---
	cout << "--- Test 27: DELETE SETS ---" << endl;
	{
		db.create_set("Temp");
		string result = parse_query(db, "DELETE SETS Temp");
		check("DELETE returns success", result.find("Deleted") != string::npos);
		check("Temp removed from index", db.set_index.find("Temp") == db.set_index.end());
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 28: GET CARDINALITY OF ---
	cout << "--- Test 28: GET CARDINALITY OF ---" << endl;
	{
		string result = parse_query(db, "GET CARDINALITY OF Animals");
		check("returns 3", result == "3");
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 29: GET CARDINALITY OF nonexistent ---
	cout << "--- Test 29: GET CARDINALITY OF nonexistent ---" << endl;
	{
		string result = parse_query(db, "GET CARDINALITY OF Ghost");
		check("returns error", result.find("Error") != string::npos);
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 30: GET CARDINALITY OF with WHERE collapse ---
	cout << "--- Test 30: GET CARDINALITY with WHERE ---" << endl;
	{
		string result = parse_query(db, "GET CARDINALITY OF Animals WHERE legs > 0");
		check("returns 2 (Dog + Cat)", result == "2");
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 31: GET CARDINALITY OF with set algebra collapse ---
	cout << "--- Test 31: GET CARDINALITY with UNION ---" << endl;
	{
		string result = parse_query(db, "GET CARDINALITY OF Animals UNION Pets");
		check("returns 4 (Dog+Cat+Fish+Hamster)", result == "4");
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 32: GET CARDINALITY with WHERE + set algebra ---
	cout << "--- Test 32: GET CARDINALITY with WHERE + UNION ---" << endl;
	{
		string result = parse_query(db, "GET CARDINALITY OF Animals WHERE legs > 0 UNION Pets WHERE legs > 0");
		check("returns 3 (Dog+Cat+Hamster)", result == "3");
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 33: IS DISJOINT (yes) ---
	cout << "--- Test 33: IS DISJOINT (yes) ---" << endl;
	{
		db.create_set("Rocks");
		db.create_set("Granite");
		db.add_member("Rocks", "Granite");
		string result = parse_query(db, "IS Animals DISJOINT Rocks");
		check("Animals disjoint Rocks = Yes", result == "Yes");
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 34: IS DISJOINT (no) ---
	cout << "--- Test 34: IS DISJOINT (no) ---" << endl;
	{
		string result = parse_query(db, "IS Animals DISJOINT Pets");
		check("Animals disjoint Pets = No", result == "No");
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 35: IS EQUIVALENT (yes) ---
	cout << "--- Test 35: IS EQUIVALENT (yes) ---" << endl;
	{
		db.create_set("AnimalsCopy");
		db.add_member("AnimalsCopy", "Dog");
		db.add_member("AnimalsCopy", "Cat");
		db.add_member("AnimalsCopy", "Fish");
		string result = parse_query(db, "IS Animals EQUAL AnimalsCopy");
		check("Animals equiv AnimalsCopy = Yes", result == "Yes");
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 36: IS EQUIVALENT (no) ---
	cout << "--- Test 36: IS EQUIVALENT (no) ---" << endl;
	{
		string result = parse_query(db, "IS Animals EQUAL Pets");
		check("Animals equiv Pets = No", result == "No");
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 37: IS PROPER SUBSET OF (yes) ---
	cout << "--- Test 37: IS PROPER SUBSET OF (yes) ---" << endl;
	{
		// Pets has Dog, Cat, Hamster. Animals has Dog, Cat, Fish.
		// Need a real proper subset: create one
		db.create_set("TwoAnimals");
		db.add_member("TwoAnimals", "Dog");
		db.add_member("TwoAnimals", "Cat");
		string result = parse_query(db, "IS TwoAnimals PROPER SUBSET OF Animals");
		check("TwoAnimals proper subset of Animals = Yes", result == "Yes");
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 38: IS PROPER SUBSET OF (no — equal sets) ---
	cout << "--- Test 38: IS PROPER SUBSET OF (no - equal) ---" << endl;
	{
		string result = parse_query(db, "IS Animals PROPER SUBSET OF AnimalsCopy");
		check("Animals proper subset of AnimalsCopy = No", result == "No");
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	// --- Test 39: IS PROPER SUBSET OF (no — not subset) ---
	cout << "--- Test 39: IS PROPER SUBSET OF (no - not subset) ---" << endl;
	{
		string result = parse_query(db, "IS Animals PROPER SUBSET OF Pets");
		check("Animals proper subset of Pets = No", result == "No");
		cout << "  Result: " << result << endl;
	}
	cout << endl;

	cout << "=== Results: " << pass_count << " passed, " << fail_count << " failed ===" << endl;
	return fail_count > 0 ? 1 : 0;
}
