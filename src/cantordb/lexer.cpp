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
		} else {
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
	} else {
		return Token {TOK_IDENTIFIER, word, -1, nullptr};
	}
}
