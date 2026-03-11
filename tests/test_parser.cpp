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

	// --- Test 3: GET SETS ---
	cout << "--- Test 3: GET SETS ---" << endl;
	{
		string result = parse_query(db, "GET SETS");
		check("returns Animals", result.find("Animals") != string::npos);
		check("returns Dog", result.find("Dog") != string::npos);
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

	cout << "=== Results: " << pass_count << " passed, " << fail_count << " failed ===" << endl;
	return fail_count > 0 ? 1 : 0;
}
