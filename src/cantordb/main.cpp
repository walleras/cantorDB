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
	cout << "SeQL queries (requires a loaded database):" << endl;
	cout << "  CREATE SETS <a> <b> ...    Create sets" << endl;
	cout << "  ADD <elem> TO <set>        Add element to set" << endl;
	cout << "  REMOVE <elem> FROM <set>   Remove element from set" << endl;
	cout << "  GET ELEMENTS OF <set>      List elements" << endl;
	cout << "  GET SETS OF <set>          List parent sets" << endl;
	cout << "  GET CARDINALITY OF <set>   Element count" << endl;
	cout << "  GET KEYS OF <set>          List property keys on a set" << endl;
	cout << "  GET KEYS OF UNIVERSAL      List all registered property names" << endl;
	cout << "  GET PROPERTY OF <set>      List all properties on a set" << endl;
	cout << "  GET PROPERTY <k> FROM <s>  Get a single property value" << endl;
	cout << "  GET COMPLEMENT OF <set>    Complement relative to universal" << endl;
	cout << "  CREATE PROPERTY <n> <type> Register a property (int, string, double, bool, long)" << endl;
	cout << "  ADD PROPERTY <k>=<v> TO <s>  Add property to set" << endl;
	cout << "  UPDATE PROPERTY <k> FROM <s> TO <v>  Update property value" << endl;
	cout << "  REMOVE PROPERTY <k> FROM <s>  Remove property from set" << endl;
	cout << "  RENAME SET <old> TO <new>  Rename a set" << endl;
	cout << "  TRASH SETS <a> <b> ...     Soft delete (move to trash)" << endl;
	cout << "  DELETE SETS <a> <b> ...    Hard delete" << endl;
	cout << "  CLEAR CACHE                Delete all cached result sets" << endl;
	cout << "  IS <a> ELEMENT OF <b>      Membership test" << endl;
	cout << "  IS <a> SUBSET OF <b>       Subset test" << endl;
	cout << "  IS <a> EQUAL <b>           Equality test" << endl;
	cout << "  IS <a> DISJOINT <b>        Disjointness test" << endl;
	cout << "  <a> UNION <b>              Set union" << endl;
	cout << "  <a> INTERSECTION <b>       Set intersection" << endl;
	cout << "  <a> DIFFERENCE <b>         Set difference" << endl;
	cout << "  <a> SYMDIFF <b>            Symmetric difference" << endl;
	cout << "  <set> WHERE <prop> > <val> Filter elements by property" << endl;
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
