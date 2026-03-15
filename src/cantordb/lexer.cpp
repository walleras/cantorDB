#include "lexer.h"


vector<Token> lexer(const string& query) {
	vector<Token> tokens;
	int i = 0;
	while (i < (int)query.size()) {
		if (isspace(query[i])) {i++; continue;}

		if (query[i] == '.') {
			// Count leading dots
			int dots_front = 0;
			while(i < (int)query.size() && query[i] == '.') {
				dots_front++;
				i++;
			}
			// Read the word between dots
			if(i < (int)query.size() && isalpha(query[i])) {
				string word = read_word(query, i);
				// Count trailing dots
				int dots_back = 0;
				while(i < (int)query.size() && query[i] == '.') {
					dots_back++;
					i++;
				}
				Token tok = classify_word(word);
				if(dots_front == dots_back) {
					tok.priority = dots_front;
				}
				tokens.push_back(tok);
			}
			continue;
		}

		if (isalpha(query[i])) {
			string word = read_word(query, i);
			tokens.push_back(classify_word(word));
		} else if(isdigit(query[i])) {
			tokens.push_back(handle_numbers(query, i));
		} else if(query[i] == ',') {

		} else {
			if(query[i] == '>') {
				if(i + 1 < (int)query.size() && query[i+1] == '=') {
					i++;
					tokens.push_back(Token {TOK_GOE, ">=", -1, nullptr});
				} else {
					tokens.push_back(Token {TOK_GREATER_THAN, ">", -1, nullptr});
				}
			} else if(query[i] == '<') {
				if(i + 1 < (int)query.size() && query[i+1] == '=') {
					i++;
					tokens.push_back(Token {TOK_LOE, "<=", -1, nullptr});
				} else {
					tokens.push_back(Token {TOK_LESSER_THAN, "<", -1, nullptr});
				}
			} else if(query[i] == '=') {
				tokens.push_back(Token {TOK_EQUALS, "=", -1, nullptr});
			}
			i++;
		}
	}

	tokens.push_back(Token {TOK_EOF, "", -1, nullptr});

	return tokens;
}

string read_word(const string& query, int& i) {
	string word;
	while (i < (int)query.size() && !isspace(query[i]) && (isalnum(query[i]) || query[i] == '_')) {
		word.push_back(query[i]);
		i++;
	}
	return word;
}

Token classify_word(string word) {
	string lower_word = word;
	transform(lower_word.begin(), lower_word.end(), lower_word.begin(), ::tolower);
	if(lower_word == "get") {
		return Token {TOK_GET, word, -1, nullptr};
	} else if(lower_word == "elements" || lower_word == "element") {
		return Token {TOK_ELEM, word, -1, nullptr};
	} else if(lower_word == "of") {
		return Token {TOK_OF, word, -1, nullptr};
	} else if(lower_word == "sets") {
		return Token {TOK_SET, word, -1, nullptr};
	} else if(lower_word == "all") {
		return Token {TOK_ALL, word, -1, nullptr};
	} else if(lower_word == "cache") {
		return Token {TOK_CACHE, word, -1, nullptr};
	} else if(lower_word == "is") {
		return Token {TOK_IS, word, -1, nullptr};
	} else if(lower_word == "subset") {
		return Token {TOK_SUB, word, -1, nullptr};
	} else if(lower_word == "superset") {
		return Token {TOK_SUPER, word, -1, nullptr};
	} else if(lower_word == "union") {
		return Token {TOK_UNION, word, 0, nullptr};
	} else if(lower_word == "intersection") {
		return Token {TOK_INTER, word, 0, nullptr};
	} else if(lower_word == "difference") {
		return Token {TOK_DIFF, word, 0, nullptr};
	} else if(lower_word == "symdiff") {
		return Token {TOK_SYMDIFF, word, 0, nullptr};
	} else if(lower_word == "where") {
		return Token {TOK_WHERE, word, -1, nullptr};
	} else if(lower_word == "filter") {
		return Token {TOK_FILTER, word, 0, nullptr};
	} else if(lower_word == "create") {
		return Token {TOK_CREATE, word, -1, nullptr};
	} else if(lower_word == "trash") {
		return Token {TOK_TRASH, word, -1, nullptr};
	} else if(lower_word == "from") {
		return Token {TOK_FROM, word, -1, nullptr};
	} else if(lower_word == "to") {
		return Token {TOK_TO, word, -1, nullptr};
	} else if(lower_word == "add") {
		return Token {TOK_ADD, word, -1, nullptr};
	} else if(lower_word == "remove") {
		return Token {TOK_REMOVE, word, -1, nullptr};
	} else if(lower_word == "delete") {
		return Token {TOK_DELETE, word, -1, nullptr};
	} else if(lower_word == "property") {
		return Token {TOK_PROPERTY, word, -1, nullptr};
	} else if(lower_word == "cardinality") {
		return Token {TOK_CARDINALITY, word, -1, nullptr};
	} else if(lower_word == "disjoint") {
		return Token {TOK_DISJOINT, word, -1, nullptr};
	} else if(lower_word == "equal") {
		return Token {TOK_EQUIV, word, -1, nullptr};
	} else if(lower_word == "proper") {
		return Token {TOK_PROPER, word, -1, nullptr};
	} else if(lower_word == "complement") {
		return Token {TOK_COMPLEMENT, word, -1, nullptr};
	} else if(lower_word == "clear") {
		return Token {TOK_CLEAR, word, -1, nullptr};
	} else if(lower_word =="update") {
		return Token {TOK_UPDATE, word, -1, nullptr};
	} else {
		return Token {TOK_IDENTIFIER, word, -1, nullptr};
	}
}

Token handle_numbers(const string& query, int& i) {
	string num;
	while(i < (int)query.size() && (isdigit(query[i]) || query[i] == '.')) {
		num.push_back(query[i]);
		i++;
	}
	return Token {TOK_NUM, num, -1, nullptr};
}
