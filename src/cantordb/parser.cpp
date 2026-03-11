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
static QueryType parse_all(vector<Token>& tokens, int& pos);
static QueryType parse_cache(vector<Token>& tokens, int& pos);
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

string parse_query(cantordb& db, const string& query) {
	vector<Token> tokens = lexer(query);
	int pos = 0;

	PEC = PE_NONE;
	parser_error = "";
	db.error_message = "";

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

	if(type == ALL_SETS) {
		return db.list_all_sets();
	}

	if(type == IS) {
		return is_query(db, tokens, pos);
	}

	if(type == CACHE_SETS) {
		return db.list_cached_sets();
	}

	return "Unknown query type.";
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
	PEC = PE_NO_GET;
	parser_error = "Syntax Error: Query must start with GET or IS.";
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
		case TOK_ALL:
			pos++;
			return parse_all(tokens, pos);
		case TOK_CACHE:
			pos++;
			return parse_cache(tokens, pos);
		default:
			PEC = PE_SYNTAX;
			parser_error = "Syntax Error: GET must be followed by ELEMENTS, SETS, ALL, or CACHE.";
			return ERR;
	}
}

static QueryType parse_elem(vector<Token>& tokens, int& pos) {
	if(tokens[pos].type == TOK_OF) {
		pos++;
		return ELEMENTS;
	}
	PEC = PE_SYNTAX;
	parser_error = "Syntax Error: ELEMENTS must be followed by OF.";
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

static QueryType parse_all(vector<Token>& tokens, int& pos) {
	if(tokens[pos].type == TOK_SET) {
		pos++;
		return ALL_SETS;
	}
	PEC = PE_SYNTAX;
	parser_error = "Syntax Error: ALL must be followed by SETS.";
	return ERR;
}

static QueryType parse_cache(vector<Token>& tokens, int& pos) {
	if(tokens[pos].type == TOK_SET) {
		pos++;
		return CACHE_SETS;
	}
	PEC = PE_SYNTAX;
	parser_error = "Syntax Error: CACHE must be followed by SETS.";
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

		// Replace [actual-1, actual, actual+1] with the result token
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
	} else {
		return "Syntax Error: Expected ELEMENT, SET, SUBSET, or SUPERSET.";
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
