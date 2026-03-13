#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

enum QueryType {ELEMENTS, SETS, ALL_SETS, CACHE_SETS, IS, ERR, CREATE, DELETE, DELETE_HARSH, ADD_ELEM, REMOVE_ELEM, ADD_PROPERTY, REMOVE_PROPERTY, CREATE_PROPERTY};

enum ParserErrorCode {PE_NONE, PE_SYNTAX, PE_NO_GET};

string parse_query(cantordb& db, const string& query);

#endif
