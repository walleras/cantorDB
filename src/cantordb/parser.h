#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

enum QueryType {ELEMENTS, SETS, ALL_SETS, CACHE_SETS, IS, ERR};

enum ParserErrorCode {PE_NONE, PE_SYNTAX, PE_NO_GET};

string parse_query(cantordb& db, const string& query);

#endif
