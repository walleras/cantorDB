#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include "cantordb.h"
#include <cctype>
#include <algorithm>

enum TokenType {TOK_EOF, TOK_ELEM, TOK_SET, TOK_OF, TOK_IDENTIFIER, TOK_GET, TOK_UNION, TOK_INTER, TOK_DIFF, TOK_SYMDIFF, TOK_ALL, TOK_CACHE, TOK_IS, TOK_SUB, TOK_SUPER};

struct Token {
	TokenType type;
	string value;
	int priority;
	Set* result;
};

vector<Token> lexer(const string& query);
string read_word(const string& query, int& i);
Token classify_word(string word);

#endif
