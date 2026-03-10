#include "cantordb.h"
#include "parser.h"
#include <string>
#include <iostream>

using namespace std;

int main() {
	cantordb* db = load_cantordb("datasets/periodic_table.cdb");
	if (!db) {
		cout << "Failed to load periodic_table.cdb" << endl;
		return 1;
	}
	cout << "Loaded database: " << db->name << endl;

	string query;
	while(true) {
		cout << ">> ";
		getline(cin, query);
		if(query == "QUIT" || query == "quit") {
			break;
		}
		string result = parse_query(*db, query);
		if (!result.empty()) {
			cout << result << endl;
		}
	}

	delete db;
	return 0;
}
