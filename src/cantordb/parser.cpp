#include "parser.h"
#include "lexer.h"
#include <string>

static ParserErrorCode PEC;
static string parser_error;

// Forward declarations
static QueryType parse_header(vector<Token>& tokens, int& pos);
static QueryType parse_query_type(vector<Token>& tokens, int& pos);
static QueryType parse_elem(vector<Token>& tokens, int& pos);
static QueryType parse_set(vector<Token>& tokens, int& pos);
static QueryType parse_all(vector<Token>& tokens, int& pos);
static QueryType parse_cache(vector<Token>& tokens, int& pos);
static void element_collapse_pass(cantordb& db, vector<Token>& tokens, int dots);
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

static void element_collapse_pass(cantordb& db, vector<Token>& tokens, int dots) {
	// TODO: collapse dot-separated identifiers at the given dot level
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
	} else if(db.EC == ER_SET_NOT_FOUND) {
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
	} else if(db.EC == ER_SET_NOT_FOUND) {
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
	} else if(db.EC == ER_SET_NOT_FOUND) {
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
	} else if(db.EC == ER_SET_NOT_FOUND) {
		return db.error_message;
	} else {
		return "No";
	}
}
