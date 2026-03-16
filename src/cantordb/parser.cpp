#include "parser.h"
#include "lexer.h"
#include <string>
#include <queue>

static ParserErrorCode PEC;
static string parser_error;

// Forward declarations
static QueryType parse_header(vector<Token>& tokens, int& pos);
static QueryType parse_query_type(vector<Token>& tokens, int& pos);
static QueryType parse_elem(vector<Token>& tokens, int& pos);
static QueryType parse_set(vector<Token>& tokens, int& pos);
static QueryType parse_cardinality(vector<Token>& tokens, int& pos);
static QueryType parse_properties(vector<Token>& tokens, int& pos);
static void element_collapse_pass(cantordb& db, vector<Token>& tokens);
static Token error_token(const string& message);
static Token union_operation(cantordb& db, vector<Token>& tokens, int operator_pos);
static Token intersection_operation(cantordb& db, vector<Token>& tokens, int operator_pos);
static Token difference_operation(cantordb& db, vector<Token>& tokens, int operator_pos);
static Token symdiff_operation(cantordb& db, vector<Token>& tokens, int operator_pos);
static string is_query(cantordb& db, vector<Token>& tokens, int& pos);
static string is_element_query(cantordb& db, vector<Token>& tokens, int& pos, string iset);
static string is_set_query(cantordb& db, vector<Token>& tokens, int& pos, string iset);
static string is_subset_query(cantordb& db, vector<Token>& tokens, int& pos, string iset);
static string is_superset_query(cantordb& db, vector<Token>& tokens, int& pos, string iset);
static string is_disjoint_query(cantordb& db, vector<Token>& tokens, int& pos, string iset);
static string is_proper_query(cantordb& db, vector<Token>& tokens, int& pos, string iset);
static string is_equiv_query(cantordb& db, vector<Token>& tokens, int& pos, string iset);
static void where_collapse_pass(cantordb& db, vector<Token>& tokens);
static Set* handle_where(cantordb& db, vector<Token>& tokens, int where_pos);
static string create_query(cantordb& db, vector<Token>& tokens, int& pos);
static string trash_query(cantordb& db, vector<Token>& tokens, int& pos);
static string delete_query(cantordb& db, vector<Token>& tokens, int& pos);
static string add_elem(cantordb& db, vector<Token>& tokens, int& pos);
static string add_property_query(cantordb& db, vector<Token>& tokens, int& pos);
static string create_property_query(cantordb& db, vector<Token>& tokens, int& pos);
static string update_property_query(cantordb& db, vector<Token>& tokens, int& pos);
static string remove_elem(cantordb& db, vector<Token>& tokens, int& pos);
static string remove_property_query(cantordb& db, vector<Token>& tokens, int& pos);
static string rename_set_query(cantordb& db, vector<Token>& tokens, int& pos);
static string rename_property_query(cantordb& db, vector<Token>& tokens, int& pos);
static QueryType parse_complement(vector<Token>& tokens, int& pos);

string parse_query(cantordb& db, const string& query) {
	vector<Token> tokens = lexer(query);
	int pos = 0;

	PEC = PE_NONE;
	parser_error = "";
	db.error_message = "";

	where_collapse_pass(db, tokens);

	// Check if collapse pass produced an error
	if(tokens.size() == 1 && tokens[0].type == TOK_EOF && !tokens[0].value.empty()) {
		return tokens[0].value;
	}

	element_collapse_pass(db, tokens);

	// Check if collapse pass produced an error
	if(tokens.size() == 1 && tokens[0].type == TOK_EOF && !tokens[0].value.empty()) {
		return tokens[0].value;
	}

	QueryType type = parse_header(tokens, pos);

	if(type == ERR) {
		return parser_error;
	}

	if(type == ELEMENTS) {
		if(tokens[pos].type != TOK_IDENTIFIER) {
			return "Syntax Error: Expected set name after OF.";
		}
		string set_name = tokens[pos].value;
		return db.list_elements(set_name);
	}

	if(type == SETS) {
		if(tokens[pos].type != TOK_IDENTIFIER) {
			return "Syntax Error: Expected set name after OF.";
		}
		string set_name = tokens[pos].value;
		return db.list_sets(set_name);
	}

	if(type == PROPERTIES) {
		if(tokens[pos].type != TOK_IDENTIFIER) {
			return "Syntax Error: Expected set name after OF.";
		}
		if(tokens[pos].result != nullptr) {
			return "Error: Cannot get properties from calculated set.";
		}
		return db.list_properties(tokens[pos].value);
	}

	if(type == GET_PROPERTY) {
		// pos is at the key name: GET PROPERTY <key> FROM <set>
		string key = tokens[pos].value;
		pos++;
		if(tokens[pos].type != TOK_FROM) {
			return "Syntax Error: Expected FROM after property name.";
		}
		pos++;
		if(tokens[pos].type != TOK_IDENTIFIER) {
			return "Syntax Error: Expected set name after FROM.";
		}
		string set_name = tokens[pos].value;
		auto result = db.get_property(set_name, key);
		if(!db.error_message.empty()) return db.error_message;
		return std::visit([](auto&& val) -> string {
			using T = std::decay_t<decltype(val)>;
			if constexpr (std::is_same_v<T, bool>) return val ? "true" : "false";
			else if constexpr (std::is_same_v<T, string>) return val;
			else return to_string(val);
		}, result);
	}

	if(type == KEYS) {
		if(tokens[pos].type == TOK_UNIVERSAL) {
			return db.list_all_keys();
		}
		if(tokens[pos].type != TOK_IDENTIFIER) {
			return "Syntax Error: Expected set name after OF.";
		}
		if(tokens[pos].result != nullptr) {
			return "Error: Cannot get keys from calculated set.";
		}
		return db.list_property_keys(tokens[pos].value);
	}

	if(type == COMPLEMENT) {
		if(tokens[pos].type != TOK_IDENTIFIER) {
			return "Syntax Error: Expected set name after OF.";
		}
		string set_name = tokens[pos].value;
		Set* comp = db.set_complement(set_name);
		if(!comp) return db.error_message;
		return db.list_elements(comp->set_name);
	}

	if(type == CARDINALITY) {
		if(tokens[pos].type != TOK_IDENTIFIER) {
			return "Syntax Error: Expected set name after OF.";
		}
		string set_name = tokens[pos].value;
		int card = db.get_cardinality(set_name);
		if(!db.error_message.empty()) return db.error_message;
		return to_string(card);
	}
	if(type == SAVE) {
		if(tokens[pos].type != TOK_IDENTIFIER) {
			return "Syntax Error: Expected file name after SAVE.";
		}
		string path = tokens[pos].value + ".cdb";
		if(save_cantordb(db, path)) {
			return "Saved database to " + path + ".";
		}
		return "Error: Failed to save database to " + path + ".";
	}
	if(type == LOAD) {
		if(tokens[pos].type != TOK_IDENTIFIER) {
			return "Syntax Error: Expected file name after LOAD.";
		}
		string path = tokens[pos].value + ".cdb";
		cantordb* loaded = load_cantordb(path);
		if(!loaded) {
			return "Error: Failed to load database from " + path + ".";
		}
		// Delete all existing sets
		for(auto& [name, s] : db.set_index) {
			delete s;
		}
		// Move loaded state into db
		db.name = loaded->name;
		db.set_index = loaded->set_index;
		db.property_types = loaded->property_types;
		db.integer_property_index = loaded->integer_property_index;
		db.string_property_index = loaded->string_property_index;
		db.double_property_index = loaded->double_property_index;
		db.bool_property_index = loaded->bool_property_index;
		db.long_property_index = loaded->long_property_index;
		db.memory_used = loaded->memory_used;
		db.emergency_shut_off = false;
		db.error_message = "";
		// Prevent loaded's destructor from freeing the sets we just took
		loaded->set_index.clear();
		delete loaded;
		return "Loaded database from " + path + ".";
	}

	if(type == CREATE) {
		return create_query(db, tokens, pos);
	}

	if(type == CREATE_PROPERTY) {
		return create_property_query(db, tokens, pos);
	}

	if(type == UPDATE) {
		return update_property_query(db, tokens, pos);
	}

	if(type == RENAME_SET) {
		return rename_set_query(db, tokens, pos);
	}

	if(type == RENAME_PROPERTY) {
		return rename_property_query(db, tokens, pos);
	}

	if(type == CLEAR_CACHE) {
		if(db.clear_cache()) return "Cache cleared.";
		return db.error_message;
	}

	if(type == DELETE) {
		return trash_query(db, tokens, pos);
	}

	if(type == IS) {
		return is_query(db, tokens, pos);
	}

	if(type == DELETE_HARSH) {
		return delete_query(db, tokens, pos);
	}

	if(type == ADD_ELEM) {
		return add_elem(db, tokens, pos);
	}

	if(type == ADD_PROPERTY) {
		return add_property_query(db, tokens, pos);
	}

	if(type == REMOVE_ELEM) {
		return remove_elem(db, tokens, pos);
	}

	if(type == REMOVE_PROPERTY) {
		return remove_property_query(db, tokens, pos);
	}

	return "Unknown query type.";
}

static void where_collapse_pass(cantordb& db, vector<Token>& tokens) {
	for(int i = 0; i < (int)tokens.size(); i++) {
		if(tokens[i].type == TOK_WHERE) {
			if(i < 1 || tokens[i - 1].type != TOK_IDENTIFIER) {
				tokens.clear();
				tokens.push_back(error_token("Syntax Error: WHERE must follow a set name."));
				return;
			}
			Set* result = handle_where(db, tokens, i);
			if(!result) {
				tokens.clear();
				tokens.push_back(error_token(db.error_message));
				return;
			}
			// Replace the set token with the filtered result
			tokens[i - 1].value = result->set_name;
			tokens[i - 1].result = result;
			// Erase from WHERE through value: [i] WHERE [i+1] property [i+2] comparator [i+3] value
			int erase_end = i + 4;
			if(erase_end > (int)tokens.size()) erase_end = (int)tokens.size();
			tokens.erase(tokens.begin() + i, tokens.begin() + erase_end);
			i--; // re-check in case of chained WHERE
		}
	}
}

static Set* dispatch_where_int(cantordb& db, const string& set_name, const string& property, TokenType comparator, int val) {
	switch(comparator) {
		case TOK_GREATER_THAN: return db.where_elements_greater_than(set_name, property, val);
		case TOK_LESSER_THAN:  return db.where_elements_lesser_than(set_name, property, val);
		case TOK_EQUALS:       return db.where_elements_equal_than(set_name, property, val);
		case TOK_GOE: {
			Set* gt = db.where_elements_greater_than(set_name, property, val);
			Set* eq = db.where_elements_equal_than(set_name, property, val);
			if(!gt || !eq) return nullptr;
			return db.set_union(gt, eq);
		}
		case TOK_LOE: {
			Set* lt = db.where_elements_lesser_than(set_name, property, val);
			Set* eq = db.where_elements_equal_than(set_name, property, val);
			if(!lt || !eq) return nullptr;
			return db.set_union(lt, eq);
		}
		default: return nullptr;
	}
}

static Set* dispatch_where_double(cantordb& db, const string& set_name, const string& property, TokenType comparator, double val) {
	switch(comparator) {
		case TOK_GREATER_THAN: return db.where_elements_greater_than(set_name, property, val);
		case TOK_LESSER_THAN:  return db.where_elements_lesser_than(set_name, property, val);
		case TOK_EQUALS:       return db.where_elements_equal_than(set_name, property, val);
		case TOK_GOE: {
			Set* gt = db.where_elements_greater_than(set_name, property, val);
			Set* eq = db.where_elements_equal_than(set_name, property, val);
			if(!gt || !eq) return nullptr;
			return db.set_union(gt, eq);
		}
		case TOK_LOE: {
			Set* lt = db.where_elements_lesser_than(set_name, property, val);
			Set* eq = db.where_elements_equal_than(set_name, property, val);
			if(!lt || !eq) return nullptr;
			return db.set_union(lt, eq);
		}
		default: return nullptr;
	}
}

static Set* dispatch_where_long(cantordb& db, const string& set_name, const string& property, TokenType comparator, long val) {
	switch(comparator) {
		case TOK_GREATER_THAN: return db.where_elements_greater_than(set_name, property, val);
		case TOK_LESSER_THAN:  return db.where_elements_lesser_than(set_name, property, val);
		case TOK_EQUALS:       return db.where_elements_equal_than(set_name, property, val);
		case TOK_GOE: {
			Set* gt = db.where_elements_greater_than(set_name, property, val);
			Set* eq = db.where_elements_equal_than(set_name, property, val);
			if(!gt || !eq) return nullptr;
			return db.set_union(gt, eq);
		}
		case TOK_LOE: {
			Set* lt = db.where_elements_lesser_than(set_name, property, val);
			Set* eq = db.where_elements_equal_than(set_name, property, val);
			if(!lt || !eq) return nullptr;
			return db.set_union(lt, eq);
		}
		default: return nullptr;
	}
}

static Set* handle_where(cantordb& db, vector<Token>& tokens, int where_pos) {
	string set_name = tokens[where_pos - 1].value;
	int p = where_pos + 1;

	// Expect: property comparator value
	if(p >= (int)tokens.size() || tokens[p].type != TOK_IDENTIFIER) {
		db.error_message = "Syntax Error: Expected property name after WHERE.";
		return nullptr;
	}
	string property = tokens[p].value;
	p++;

	if(p >= (int)tokens.size()) {
		db.error_message = "Syntax Error: Expected comparator after property.";
		return nullptr;
	}
	TokenType comparator = tokens[p].type;
	if(comparator != TOK_GREATER_THAN && comparator != TOK_LESSER_THAN &&
	   comparator != TOK_EQUALS && comparator != TOK_GOE && comparator != TOK_LOE) {
		db.error_message = "Syntax Error: Expected >, <, =, >=, or <= after property.";
		return nullptr;
	}
	p++;

	if(p >= (int)tokens.size() || (tokens[p].type != TOK_NUM && tokens[p].type != TOK_IDENTIFIER)) {
		db.error_message = "Syntax Error: Expected value after comparator.";
		return nullptr;
	}

	// String/bool equality
	if(tokens[p].type == TOK_IDENTIFIER) {
		if(comparator != TOK_EQUALS) {
			db.error_message = "Syntax Error: Only = is supported for string and bool comparisons.";
			return nullptr;
		}
		string val = tokens[p].value;
		if(val == "true" || val == "True" || val == "TRUE") {
			return db.where_elements_equal_than(set_name, property, true);
		} else if(val == "false" || val == "False" || val == "FALSE") {
			return db.where_elements_equal_than(set_name, property, false);
		} else {
			return db.where_elements_equal_than(set_name, property, val);
		}
	}

	// Numeric value — dispatch based on which property index actually holds this property,
	// not based on how the user wrote the literal
	double num_val = stod(tokens[p].value);

	if(db.integer_property_index.find(property) != db.integer_property_index.end()) {
		return dispatch_where_int(db, set_name, property, comparator, (int)num_val);
	}
	if(db.double_property_index.find(property) != db.double_property_index.end()) {
		return dispatch_where_double(db, set_name, property, comparator, num_val);
	}
	if(db.long_property_index.find(property) != db.long_property_index.end()) {
		return dispatch_where_long(db, set_name, property, comparator, (long)num_val);
	}

	db.error_message = "Error: No numeric property \"" + property + "\" found in any index.";
	return nullptr;
}

static string create_query(cantordb& db, vector<Token>& tokens, int& pos) {
	if(tokens[pos].type != TOK_SET) {
		return "Syntax Error: CREATE must be followed by SETS.";
	}
	pos++;
	int count = 0;
	while(tokens[pos].type == TOK_IDENTIFIER) {
		if(!db.create_set(tokens[pos].value)) {
			return db.error_message;
		}
		count++;
		pos++;
	}
	if(count == 0) {
		return "Syntax Error: Expected at least one set name after CREATE SETS.";
	}
	return "Created " + to_string(count) + " set" + (count > 1 ? "s" : "") + ".";
}

static string rename_set_query(cantordb& db, vector<Token>& tokens, int& pos) {
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: RENAME SET must be followed by a set name.";
	}
	string old_name = tokens[pos].value;
	pos++;
	if(tokens[pos].type != TOK_TO) {
		return "Syntax Error: Expected TO after set name in RENAME SET.";
	}
	pos++;
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected new set name after TO.";
	}
	string new_name = tokens[pos].value;
	pos++;
	if(db.rename_set(old_name, new_name)) {
		return "Renamed set \"" + old_name + "\" to \"" + new_name + "\".";
	}
	return db.error_message;
}

static string rename_property_query(cantordb& db, vector<Token>& tokens, int& pos) {
	return "Not yet implemented.";
}

static string trash_query(cantordb& db, vector<Token>& tokens, int& pos) {
	if(tokens[pos].type != TOK_SET) {
		return "Syntax Error: TRASH must be followed by SETS.";
	}
	pos++;
	int count = 0;
	while(tokens[pos].type == TOK_IDENTIFIER) {
		if(!db.delete_set(tokens[pos].value, true)) {
			return db.error_message;
		}
		count++;
		pos++;
	}
	if(count == 0) {
		return "Syntax Error: Expected at least one set name after TRASH SETS.";
	}
	return "Trashed " + to_string(count) + " set" + (count > 1 ? "s" : "") + ".";
}

static string delete_query(cantordb& db, vector<Token>& tokens, int& pos) {
	if(tokens[pos].type != TOK_SET) {
		return "Syntax Error: DELETE must be followed by SETS.";
	}
	pos++;
	int count = 0;
	while(tokens[pos].type == TOK_IDENTIFIER) {
		if(!db.delete_set(tokens[pos].value, false)) {
			return db.error_message;
		}
		count++;
		pos++;
	}
	if(count == 0) {
		return "Syntax Error: Expected at least one set name after DELETE SETS.";
	}
	return "Deleted " + to_string(count) + " set" + (count > 1 ? "s" : "") + ".";
}

// ADD Dog TO Animals
static string add_elem(cantordb& db, vector<Token>& tokens, int& pos) {
	// pos is at the element name (parse_header consumed ADD, saw IDENTIFIER)
	string elem_name = tokens[pos].value;
	pos++;
	if(tokens[pos].type != TOK_TO) {
		return "Syntax Error: Expected TO after element name.";
	}
	pos++;
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected set name after TO.";
	}
	string set_name = tokens[pos].value;
	if(!db.add_member(set_name, elem_name)) {
		return db.error_message;
	}
	return "Added " + elem_name + " to " + set_name + ".";
}

// UPDATE PROPERTY key FROM Set TO value
static string update_property_query(cantordb& db, vector<Token>& tokens, int& pos) {
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected property name after PROPERTY.";
	}
	string key = tokens[pos].value;
	pos++;

	if(tokens[pos].type != TOK_FROM) {
		return "Syntax Error: Expected FROM after property name.";
	}
	pos++;

	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected set name after FROM.";
	}
	string set_name = tokens[pos].value;
	pos++;

	if(tokens[pos].type != TOK_TO) {
		return "Syntax Error: Expected TO after set name.";
	}
	pos++;

	// Check that the set exists
	if(db.set_index.find(set_name) == db.set_index.end()) {
		return "Error: Set \"" + set_name + "\" not found.";
	}

	// Check that the property exists on the set
	Set* s = db.set_index[set_name];
	if(s->key_index.find(key) == s->key_index.end()) {
		return "Error: Key \"" + key + "\" not found in set \"" + set_name + "\".";
	}

	int type_tag = s->key_index[key];

	// Read the value token
	if(tokens[pos].type != TOK_NUM && tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected value after TO.";
	}

	// Dispatch by the existing property type
	switch(type_tag) {
		case 0: { // int
			if(tokens[pos].type != TOK_NUM) {
				return "Syntax Error: Property \"" + key + "\" is int type, expected a number.";
			}
			int val = (int)stod(tokens[pos].value);
			if(!db.update_property(set_name, key, val)) {
				return db.error_message;
			}
			break;
		}
		case 1: { // string
			if(tokens[pos].type != TOK_IDENTIFIER) {
				return "Syntax Error: Property \"" + key + "\" is string type, expected a string value.";
			}
			string val = tokens[pos].value;
			if(!db.update_property(set_name, key, val)) {
				return db.error_message;
			}
			break;
		}
		case 2: { // double
			if(tokens[pos].type != TOK_NUM) {
				return "Syntax Error: Property \"" + key + "\" is double type, expected a number.";
			}
			double val = stod(tokens[pos].value);
			if(!db.update_property(set_name, key, val)) {
				return db.error_message;
			}
			break;
		}
		case 3: { // bool
			if(tokens[pos].type != TOK_IDENTIFIER) {
				return "Syntax Error: Property \"" + key + "\" is bool type, expected true or false.";
			}
			string v = tokens[pos].value;
			bool val;
			if(v == "true" || v == "True" || v == "TRUE") {
				val = true;
			} else if(v == "false" || v == "False" || v == "FALSE") {
				val = false;
			} else {
				return "Syntax Error: Property \"" + key + "\" is bool type, expected true or false.";
			}
			if(!db.update_property(set_name, key, val)) {
				return db.error_message;
			}
			break;
		}
		case 4: { // long
			if(tokens[pos].type != TOK_NUM) {
				return "Syntax Error: Property \"" + key + "\" is long type, expected a number.";
			}
			long val = stol(tokens[pos].value);
			if(!db.update_property(set_name, key, val)) {
				return db.error_message;
			}
			break;
		}
		default:
			return "Error: Unknown property type for key \"" + key + "\".";
	}

	return "Updated property " + key + " on " + set_name + ".";
}

static string create_property_query(cantordb& db, vector<Token>& tokens, int& pos) {
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected property name after CREATE PROPERTY.";
	}
	string prop_name = tokens[pos].value;
	pos++;
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected type (int, string, double, bool, long) after property name.";
	}
	string type_name = tokens[pos].value;
	transform(type_name.begin(), type_name.end(), type_name.begin(), ::tolower);
	pos++;
	if(db.create_property(prop_name, type_name)) {
		return "Created property \"" + prop_name + "\" with type " + type_name + ".";
	}
	return db.error_message;
}

// ADD PROPERTY <key> TO <set>            — zero-initializes as int 0
// ADD PROPERTY <key> = <value> TO <set>  — infers type from value
static string add_property_query(cantordb& db, vector<Token>& tokens, int& pos) {
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected property name after PROPERTY.";
	}
	string key = tokens[pos].value;
	pos++;

	// Check for optional = value
	bool has_value = false;
	string str_val;
	double num_val = 0;
	bool is_num = false;
	bool is_bool = false;
	bool bool_val = false;

	if(tokens[pos].type == TOK_EQUALS) {
		has_value = true;
		pos++;
		if(tokens[pos].type == TOK_NUM) {
			num_val = stod(tokens[pos].value);
			is_num = true;
			pos++;
		} else if(tokens[pos].type == TOK_IDENTIFIER) {
			str_val = tokens[pos].value;
			if(str_val == "true" || str_val == "True" || str_val == "TRUE") {
				is_bool = true;
				bool_val = true;
			} else if(str_val == "false" || str_val == "False" || str_val == "FALSE") {
				is_bool = true;
				bool_val = false;
			}
			pos++;
		} else {
			return "Syntax Error: Expected value after =.";
		}
	}

	if(tokens[pos].type != TOK_TO) {
		return "Syntax Error: Expected TO after property value.";
	}
	pos++;

	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected set name after TO.";
	}
	string set_name = tokens[pos].value;

	if(!has_value) {
		// Zero-initialize using 2-arg overload (respects registered type)
		if(!db.add_property(key, set_name)) {
			return db.error_message;
		}
	} else if(is_bool) {
		if(!db.add_property(key, bool_val, set_name)) {
			return db.error_message;
		}
	} else if(is_num) {
		// Check if it has a decimal point
		if(tokens[pos - 2].value.find('.') != string::npos) {
			if(!db.add_property(key, num_val, set_name)) {
				return db.error_message;
			}
		} else {
			// Check registered type to distinguish int vs long
			auto it = db.property_types.find(key);
			if(it != db.property_types.end() && it->second == LONG) {
				if(!db.add_property(key, (long)num_val, set_name)) {
					return db.error_message;
				}
			} else {
				if(!db.add_property(key, (int)num_val, set_name)) {
					return db.error_message;
				}
			}
		}
	} else {
		if(!db.add_property(key, str_val, set_name)) {
			return db.error_message;
		}
	}

	return "Added property " + key + " to " + set_name + ".";
}

// REMOVE Dog FROM Animals
static string remove_elem(cantordb& db, vector<Token>& tokens, int& pos) {
	// pos is at the element name (parse_header consumed REMOVE, saw IDENTIFIER)
	string elem_name = tokens[pos].value;
	pos++;
	if(tokens[pos].type != TOK_FROM) {
		return "Syntax Error: Expected FROM after element name.";
	}
	pos++;
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected set name after FROM.";
	}
	string set_name = tokens[pos].value;
	if(!db.remove_member(set_name, elem_name)) {
		return db.error_message;
	}
	return "Removed " + elem_name + " from " + set_name + ".";
}

// REMOVE PROPERTY name FROM Dog
static string remove_property_query(cantordb& db, vector<Token>& tokens, int& pos) {
	// pos is at the key name (parse_header consumed REMOVE PROPERTY)
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected property name after PROPERTY.";
	}
	string key = tokens[pos].value;
	pos++;
	if(tokens[pos].type != TOK_FROM) {
		return "Syntax Error: Expected FROM after property name.";
	}
	pos++;
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected set name after FROM.";
	}
	string set_name = tokens[pos].value;
	if(!db.delete_property(set_name, key)) {
		return db.error_message;
	}
	return "Removed property " + key + " from " + set_name + ".";
}

static QueryType parse_header(vector<Token>& tokens, int& pos) {
	if(tokens[pos].type == TOK_GET) {
		pos++;
		return parse_query_type(tokens, pos);
	}
	if(tokens[pos].type == TOK_IS) {
		pos++;
		return IS;
	}
	if(tokens[pos].type == TOK_CREATE) {
		pos++;
		if(tokens[pos].type == TOK_PROPERTY) {
			pos++;
			return CREATE_PROPERTY;
		}
		return CREATE;
	}
	if(tokens[pos].type == TOK_SAVE) {
		pos++;
		return SAVE;
	}
	if(tokens[pos].type == TOK_LOAD) {
		pos++;
		return LOAD;
	}
	if(tokens[pos].type == TOK_TRASH) {
		pos++;
		return DELETE;
	}
	if(tokens[pos].type == TOK_DELETE) {
		pos++;
		return DELETE_HARSH;
	}
	if(tokens[pos].type == TOK_ADD) {
		pos++;
		if(tokens[pos].type == TOK_IDENTIFIER) {
			return ADD_ELEM;
		} else if (tokens[pos].type == TOK_PROPERTY) {
			pos++;
			return ADD_PROPERTY;
		}
	}
	if(tokens[pos].type == TOK_REMOVE) {
		pos++;
		if(tokens[pos].type == TOK_IDENTIFIER) {
			return REMOVE_ELEM;
		} else if (tokens[pos].type == TOK_PROPERTY) {
			pos++;
			return REMOVE_PROPERTY;
		}
	}
	if(tokens[pos].type == TOK_CLEAR) {
		pos++;
		if(tokens[pos].type == TOK_CACHE) {
			pos++;
			return CLEAR_CACHE;
		}
		PEC = PE_SYNTAX;
		parser_error = "Syntax Error: CLEAR must be followed by CACHE.";
		return ERR;
	}
	if(tokens[pos].type == TOK_RENAME) {
		pos++;
		if(tokens[pos].type == TOK_SET) {
			pos++;
			return RENAME_SET;
		} else if(tokens[pos].type == TOK_PROPERTY) {
			pos++;
			return RENAME_PROPERTY;
		}
		PEC = PE_SYNTAX;
		parser_error = "Syntax Error: RENAME must be followed by SET or PROPERTY.";
		return ERR;
	}
	if(tokens[pos].type == TOK_UPDATE) {
		pos++;
		if(tokens[pos].type == TOK_PROPERTY) {
			pos++;
			return UPDATE;
		}
		PEC = PE_SYNTAX;
		parser_error = "Syntax Error: UPDATE must be followed by PROPERTY.";
		return ERR;
	}
	PEC = PE_NO_GET;
	parser_error = "Syntax Error: Query must start with a valid operation.";
	return ERR;
}

static QueryType parse_query_type(vector<Token>& tokens, int& pos) {
	switch(tokens[pos].type) {
		case TOK_ELEM:
			pos++;
			return parse_elem(tokens, pos);
		case TOK_SET:
			pos++;
			return parse_set(tokens, pos);
		case TOK_CARDINALITY:
			pos++;
			return parse_cardinality(tokens, pos);
		case TOK_PROPERTY:
			pos++;
			return parse_properties(tokens, pos);
		case TOK_KEY:
			pos++;
			if(tokens[pos].type == TOK_OF) {
				pos++;
				return KEYS;
			}
			PEC = PE_SYNTAX;
			parser_error = "Syntax Error: KEYS must be followed by OF.";
			return ERR;
		case TOK_COMPLEMENT:
			pos++;
			return parse_complement(tokens, pos);
		default:
			PEC = PE_SYNTAX;
			parser_error = "Syntax Error: GET must be followed by ELEMENTS, SETS, KEYS, or COMPLEMENT.";
			return ERR;
	}
}

static QueryType parse_complement(vector<Token>& tokens, int& pos) {
	if(tokens[pos].type == TOK_OF) {
		pos++;
		return COMPLEMENT;
	}
	PEC = PE_SYNTAX;
	parser_error = "Syntax Error: COMPLEMENT must be followed by OF.";
	return ERR;
}

static QueryType parse_elem(vector<Token>& tokens, int& pos) {
	if(tokens[pos].type == TOK_OF) {
		pos++;
		if(tokens[pos].type == TOK_UNIVERSAL) {
			tokens[pos].type = TOK_IDENTIFIER;
			tokens[pos].value = "__universal__";
			return ELEMENTS;
		}
		if(tokens[pos].type == TOK_CACHE) {
			tokens[pos].type = TOK_IDENTIFIER;
			tokens[pos].value = "__cache__";
			return ELEMENTS;
		}
		return ELEMENTS;
	}
	PEC = PE_SYNTAX;
	parser_error = "Syntax Error: ELEMENTS must be followed by OF.";
	return ERR;
}

static QueryType parse_properties(vector<Token>& tokens, int& pos) {
	if(tokens[pos].type == TOK_OF) {
		pos++;
		return PROPERTIES;
	}
	if(tokens[pos].type == TOK_IDENTIFIER) {
		return GET_PROPERTY;
	}
	PEC = PE_SYNTAX;
	parser_error = "Syntax Error: PROPERTY must be followed by OF or a property name.";
	return ERR;
}

static QueryType parse_cardinality(vector<Token>& tokens, int& pos) {
	if(tokens[pos].type == TOK_OF) {
		pos++;
		return CARDINALITY;
	}
	PEC = PE_SYNTAX;
	parser_error = "Syntax Error: CARDINALITY must be followed by OF.";
	return ERR;
}

static QueryType parse_set(vector<Token>& tokens, int& pos) {
	if(tokens[pos].type == TOK_OF) {
		pos++;
		return SETS;
	}
	PEC = PE_SYNTAX;
	parser_error = "Syntax Error: SETS must be followed by OF.";
	return ERR;
}

static void element_collapse_pass(cantordb& db, vector<Token>& tokens) {
	// Build a max-heap of {priority, position} for all operators
	priority_queue<pair<int,int>> pq;

	for(int i = 0; i < (int)tokens.size(); i++) {
		if(tokens[i].priority >= 0) {
			pq.push({tokens[i].priority, i});
		}
	}

	while(!pq.empty()) {
		auto [pri, pos] = pq.top();
		pq.pop();

		// Position may have shifted due to earlier collapses — find the actual
		// operator by scanning for the first token at this priority
		int actual = -1;
		for(int i = 0; i < (int)tokens.size(); i++) {
			if(tokens[i].priority == pri && (tokens[i].type == TOK_UNION ||
			    tokens[i].type == TOK_INTER || tokens[i].type == TOK_DIFF ||
			    tokens[i].type == TOK_SYMDIFF)) {
				actual = i;
				break;
			}
		}
		if(actual == -1) continue;

		Token result;
		switch(tokens[actual].type) {
			case TOK_UNION:  result = union_operation(db, tokens, actual); break;
			case TOK_INTER:  result = intersection_operation(db, tokens, actual); break;
			case TOK_DIFF:   result = difference_operation(db, tokens, actual); break;
			case TOK_SYMDIFF: result = symdiff_operation(db, tokens, actual); break;
			default: continue;
		}

		if(result.type == TOK_EOF) {
			// Error — replace everything with the error token
			tokens.clear();
			tokens.push_back(result);
			return;
		}

		tokens.erase(tokens.begin() + actual - 1, tokens.begin() + actual + 2);
		tokens.insert(tokens.begin() + actual - 1, result);
	}
}

static Token error_token(const string& message) {
	return Token {TOK_EOF, message, -1, nullptr};
}

static Token union_operation(cantordb& db, vector<Token>& tokens, int operator_pos) {
	if(operator_pos < 1 || operator_pos + 1 >= (int)tokens.size()) {
		return error_token("Syntax Error: UNION requires a set on each side.");
	}
	if(tokens[operator_pos - 1].type != TOK_IDENTIFIER || tokens[operator_pos + 1].type != TOK_IDENTIFIER) {
		return error_token("Syntax Error: UNION requires a set name on each side.");
	}
	Set* result = db.set_union(tokens[operator_pos - 1].value, tokens[operator_pos + 1].value);
	if(!result) {
		return error_token(db.error_message);
	}
	return Token {TOK_IDENTIFIER, result->set_name, -1, result};
}

static Token intersection_operation(cantordb& db, vector<Token>& tokens, int operator_pos) {
	if(operator_pos < 1 || operator_pos + 1 >= (int)tokens.size()) {
		return error_token("Syntax Error: INTERSECTION requires a set on each side.");
	}
	if(tokens[operator_pos - 1].type != TOK_IDENTIFIER || tokens[operator_pos + 1].type != TOK_IDENTIFIER) {
		return error_token("Syntax Error: INTERSECTION requires a set name on each side.");
	}
	Set* result = db.set_intersection(tokens[operator_pos - 1].value, tokens[operator_pos + 1].value);
	if(!result) {
		return error_token(db.error_message);
	}
	return Token {TOK_IDENTIFIER, result->set_name, -1, result};
}

static Token difference_operation(cantordb& db, vector<Token>& tokens, int operator_pos) {
	if(operator_pos < 1 || operator_pos + 1 >= (int)tokens.size()) {
		return error_token("Syntax Error: DIFFERENCE requires a set on each side.");
	}
	if(tokens[operator_pos - 1].type != TOK_IDENTIFIER || tokens[operator_pos + 1].type != TOK_IDENTIFIER) {
		return error_token("Syntax Error: DIFFERENCE requires a set name on each side.");
	}
	Set* result = db.set_difference(tokens[operator_pos - 1].value, tokens[operator_pos + 1].value);
	if(!result) {
		return error_token(db.error_message);
	}
	return Token {TOK_IDENTIFIER, result->set_name, -1, result};
}

static Token symdiff_operation(cantordb& db, vector<Token>& tokens, int operator_pos) {
	if(operator_pos < 1 || operator_pos + 1 >= (int)tokens.size()) {
		return error_token("Syntax Error: SYMDIFF requires a set on each side.");
	}
	if(tokens[operator_pos - 1].type != TOK_IDENTIFIER || tokens[operator_pos + 1].type != TOK_IDENTIFIER) {
		return error_token("Syntax Error: SYMDIFF requires a set name on each side.");
	}
	Set* result = db.symmetric_difference(tokens[operator_pos - 1].value, tokens[operator_pos + 1].value);
	if(!result) {
		return error_token(db.error_message);
	}
	return Token {TOK_IDENTIFIER, result->set_name, -1, result};
}

static string is_query(cantordb& db, vector<Token>& tokens, int& pos) {
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: IS must be followed by a set name.";
	}
	string iset = tokens[pos].value;
	pos++;
	if(tokens[pos].type == TOK_ELEM) {
		pos++;
		return is_element_query(db, tokens, pos, iset);
	} else if (tokens[pos].type == TOK_SET) {
		pos++;
		return is_set_query(db, tokens, pos, iset);
	} else if (tokens[pos].type == TOK_SUB) {
		pos++;
		return is_subset_query(db, tokens, pos, iset);
	} else if (tokens[pos].type == TOK_SUPER) {
		pos++;
		return is_superset_query(db, tokens, pos, iset);
	} else if (tokens[pos].type == TOK_DISJOINT) {
		pos++;
		return is_disjoint_query(db, tokens, pos, iset);
	} else if (tokens[pos].type == TOK_PROPER) {
		pos++;
		return is_proper_query(db, tokens, pos, iset);
	} else if (tokens[pos].type == TOK_EQUIV) {
		pos++;
		return is_equiv_query(db, tokens, pos, iset);
	} else {
		return "Syntax Error: Expected ELEMENT, SET, SUBSET, SUPERSET, DISJOINT, PROPER, or EQUIVALENT.";
	}
}

static string is_element_query(cantordb& db, vector<Token>& tokens, int& pos, string iset) {
	if(tokens[pos].type != TOK_OF) {
		return "Syntax Error: Expected OF after ELEMENT.";
	}
	pos++;
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected set name after OF.";
	}
	if(db.is_element(iset, tokens[pos].value)) {
		return "Yes";
	} else if(!db.error_message.empty()) {
		return db.error_message;
	} else {
		return "No";
	}
}

static string is_set_query(cantordb& db, vector<Token>& tokens, int& pos, string iset) {
	if(tokens[pos].type != TOK_OF) {
		return "Syntax Error: Expected OF after SET.";
	}
	pos++;
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected set name after OF.";
	}
	if(db.is_element(tokens[pos].value, iset)) {
		return "Yes";
	} else if(!db.error_message.empty()) {
		return db.error_message;
	} else {
		return "No";
	}
}

static string is_subset_query(cantordb& db, vector<Token>& tokens, int& pos, string iset) {
	if(tokens[pos].type != TOK_OF) {
		return "Syntax Error: Expected OF after SUBSET.";
	}
	pos++;
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected set name after OF.";
	}
	if(db.is_subset(iset, tokens[pos].value)) {
		return "Yes";
	} else if(!db.error_message.empty()) {
		return db.error_message;
	} else {
		return "No";
	}
}

static string is_superset_query(cantordb& db, vector<Token>& tokens, int& pos, string iset) {
	if(tokens[pos].type != TOK_OF) {
		return "Syntax Error: Expected OF after SUPERSET.";
	}
	pos++;
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected set name after OF.";
	}
	if(db.is_superset(iset, tokens[pos].value)) {
		return "Yes";
	} else if(!db.error_message.empty()) {
		return db.error_message;
	} else {
		return "No";
	}
}

static string is_disjoint_query(cantordb& db, vector<Token>& tokens, int& pos, string iset) {
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected set name after DISJOINT.";
	}
	if(db.is_disjoint(iset, tokens[pos].value)) {
		return "Yes";
	} else if(!db.error_message.empty()) {
		return db.error_message;
	} else {
		return "No";
	}
}

static string is_proper_query(cantordb& db, vector<Token>& tokens, int& pos, string iset) {
	if(tokens[pos].type != TOK_SUB) {
		return "Syntax Error: Expected SUBSET after PROPER.";
	}
	pos++;
	if(tokens[pos].type != TOK_OF) {
		return "Syntax Error: Expected OF after PROPER SUBSET.";
	}
	pos++;
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected set name after OF.";
	}
	if(db.is_proper_subset(iset, tokens[pos].value)) {
		return "Yes";
	} else if(!db.error_message.empty()) {
		return db.error_message;
	} else {
		return "No";
	}
}

static string is_equiv_query(cantordb& db, vector<Token>& tokens, int& pos, string iset) {
	if(tokens[pos].type != TOK_IDENTIFIER) {
		return "Syntax Error: Expected set name after EQUIVALENT.";
	}
	if(db.is_equal(iset, tokens[pos].value)) {
		return "Yes";
	} else if(!db.error_message.empty()) {
		return db.error_message;
	} else {
		return "No";
	}
}