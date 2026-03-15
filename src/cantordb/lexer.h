#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include "cantordb.h"
#include <cctype>
#include <algorithm>

enum TokenType {TOK_EOF, TOK_ELEM, TOK_SET, TOK_OF, TOK_TO, TOK_FROM, TOK_IDENTIFIER, TOK_GET, TOK_UNION, TOK_INTER, TOK_DIFF, TOK_SYMDIFF, TOK_ALL, TOK_CACHE, TOK_IS, TOK_SUB, TOK_SUPER, TOK_EQUALS, TOK_GREATER_THAN, TOK_LESSER_THAN, TOK_LOE, TOK_GOE, TOK_NUM, TOK_WHERE, TOK_FILTER, TOK_CREATE, TOK_TRASH, TOK_ADD, TOK_REMOVE, TOK_DELETE,
TOK_PROPERTY, TOK_INTEGER, TOK_STRING, TOK_DECIMAL, TOK_LONG, TOK_BOOL, TOK_COMMA, TOK_CARDINALITY, TOK_DISJOINT, TOK_EQUIV, TOK_PROPER, TOK_COMPLEMENT, TOK_CLEAR, TOK_RENAME, TOK_UPDATE};

struct Token {
	TokenType type;
	string value;
	int priority;
	Set* result;
};

vector<Token> lexer(const string& query);
string read_word(const string& query, int& i);
Token classify_word(string word);
Token handle_numbers(const string& query, int& i);

#endif
