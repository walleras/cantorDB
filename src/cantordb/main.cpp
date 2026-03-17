#include "cantordb.h"
#include "parser.h"
#include <string>
#include <iostream>

using namespace std;

static void print_help() {
	cout << "Shell commands:" << endl;
	cout << "  CREATE DATABASE <name>     Create a new empty database" << endl;
	cout << "  LOAD <filename>            Load a .cdb file" << endl;
	cout << "  SAVE <filename>            Save current database to .cdb file" << endl;
	cout << "  UNLOAD                     Close the current database" << endl;
	cout << "  HELP                       Show this message" << endl;
	cout << "  QUIT                       Exit" << endl;
	cout << endl;
	cout << "Built-in sets:" << endl;
	cout << "  UNIVERSAL                  Contains every user-created set" << endl;
	cout << "  CACHE                      Contains result sets from set operations" << endl;
	cout << "  TRASH                      Contains soft-deleted sets" << endl;
	cout << "  (These cannot be renamed, trashed, or deleted.)" << endl;
	cout << endl;
	cout << "NOTE: Set names are CASE SENSITIVE. \"Animals\" and \"animals\" are different sets." << endl;
	cout << endl;
	cout << "SeQL queries (requires a loaded database):" << endl;
	cout << endl;
	cout << "  Creating & modifying sets:" << endl;
	cout << "    CREATE SETS <a> <b> ...          Create one or more sets" << endl;
	cout << "    ADD <elem> TO <set>              Add element to set" << endl;
	cout << "    REMOVE <elem> FROM <set>         Remove element from set" << endl;
	cout << "    RENAME SET <old> TO <new>        Rename a set" << endl;
	cout << "    TRASH SETS <a> <b> ...           Soft delete (move to trash)" << endl;
	cout << "    DELETE SETS <a> <b> ...          Hard delete" << endl;
	cout << "    CLEAR CACHE                      Delete all cached result sets" << endl;
	cout << endl;
	cout << "  Querying sets:" << endl;
	cout << "    GET ELEMENTS OF <set>            List elements of a set" << endl;
	cout << "    GET ELEMENTS OF UNIVERSAL        List all user-created sets" << endl;
	cout << "    GET ELEMENTS OF CACHE            List cached result sets" << endl;
	cout << "    GET ELEMENTS OF TRASH            List trashed sets" << endl;
	cout << "    GET SETS OF <set>                List parent sets a set belongs to" << endl;
	cout << "    GET CARDINALITY OF <set>         Element count" << endl;
	cout << "    GET COMPLEMENT OF <set>          Complement relative to UNIVERSAL" << endl;
	cout << endl;
	cout << "  Properties:" << endl;
	cout << "    CREATE PROPERTY <name> <type>    Register a property (int, string, double, bool, long)" << endl;
	cout << "    ADD PROPERTY <k>=<v> TO <set>    Add property to set" << endl;
	cout << "    UPDATE PROPERTY <k> FROM <s> TO <v>  Update property value" << endl;
	cout << "    REMOVE PROPERTY <k> FROM <set>   Remove property from set" << endl;
	cout << "    GET KEYS OF <set>                List property keys on a set" << endl;
	cout << "    GET KEYS OF UNIVERSAL            List all registered property names" << endl;
	cout << "    GET PROPERTY OF <set>            List all properties on a set" << endl;
	cout << "    GET PROPERTY <key> FROM <set>    Get a single property value" << endl;
	cout << endl;
	cout << "  Comparisons:            left  OP  right" << endl;
	cout << "    IS <a> ELEMENT OF <b>        Is a an element of b?" << endl;
	cout << "    IS <a> SUBSET OF <b>         Is a a subset of b?" << endl;
	cout << "    IS <a> EQUAL <b>             Are a and b equal?" << endl;
	cout << "    IS <a> DISJOINT <b>          Do a and b share no elements?" << endl;
	cout << endl;
	cout << "  Set operations:         left  OP  right  -->  result (cached)" << endl;
	cout << "    <a> UNION <b>                Elements in a or b" << endl;
	cout << "    <a> INTERSECTION <b>         Elements in both a and b" << endl;
	cout << "    <a> DIFFERENCE <b>           Elements in a but not b" << endl;
	cout << "    <a> SYMDIFF <b>              Elements in a or b but not both" << endl;
	cout << "    <set> WHERE <prop> > <val>   Filter elements by property" << endl;
}

int main() {
	cantordb* db = nullptr;

	cout << "CantorDB Shell" << endl;
	cout << "Type HELP for a list of commands." << endl;
	cout << "Try: LOAD datasets/periodic_table" << endl;
	cout << endl;

	string query;
	while(true) {
		if(db) {
			cout << db->name << " >> ";
		} else {
			cout << "cantordb >> ";
		}
		getline(cin, query);

		if(query.empty()) continue;

		// Shell-level commands (work without a loaded database)
		if(query == "QUIT" || query == "quit") {
			break;
		}

		if(query == "HELP" || query == "help") {
			print_help();
			continue;
		}

		if(query.substr(0, 6) == "CREATE" && query.find("DATABASE") != string::npos) {
			// CREATE DATABASE <name>
			size_t pos = query.find("DATABASE") + 9;
			if(pos >= query.size()) {
				cout << "Usage: CREATE DATABASE <name>" << endl;
				continue;
			}
			string name = query.substr(pos);
			while(!name.empty() && name.back() == ' ') name.pop_back();
			while(!name.empty() && name.front() == ' ') name.erase(name.begin());
			if(name.empty()) {
				cout << "Usage: CREATE DATABASE <name>" << endl;
				continue;
			}
			delete db;
			db = new cantordb(name);
			cout << "Created database: " << name << endl;
			continue;
		}

		if(query.substr(0, 4) == "LOAD" || query.substr(0, 4) == "load") {
			size_t pos = 5;
			if(pos >= query.size()) {
				cout << "Usage: LOAD <filename>" << endl;
				continue;
			}
			string path = query.substr(pos);
			while(!path.empty() && path.back() == ' ') path.pop_back();
			while(!path.empty() && path.front() == ' ') path.erase(path.begin());
			if(path.find(".cdb") == string::npos) path += ".cdb";
			cantordb* loaded = load_cantordb(path);
			if(!loaded) {
				cout << "Error: Failed to load " << path << endl;
				continue;
			}
			delete db;
			db = loaded;
			cout << "Loaded database: " << db->name << endl;
			continue;
		}

		if(query == "UNLOAD" || query == "unload") {
			if(!db) {
				cout << "No database loaded." << endl;
				continue;
			}
			cout << "Unloaded database: " << db->name << endl;
			delete db;
			db = nullptr;
			continue;
		}

		// Everything below requires a loaded database
		if(!db) {
			cout << "No database loaded. Use CREATE DATABASE <name> or LOAD <filename>." << endl;
			continue;
		}

		string result = parse_query(*db, query);
		if(!result.empty()) {
			cout << result << endl;
		}
	}

	delete db;
	return 0;
}
