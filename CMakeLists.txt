cmake_minimum_required(VERSION 3.16)
project(cantordb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Target 1: Core C++ API (no parser/lexer) ---
add_library(cantordb STATIC
	src/cantordb/cantordb.cpp
)
target_include_directories(cantordb PUBLIC src/cantordb)

# --- Target 2: Core + SeQL (parser/lexer) ---
add_library(cantordb_seql STATIC
	src/cantordb/cantordb.cpp
	src/cantordb/lexer.cpp
	src/cantordb/parser.cpp
)
target_include_directories(cantordb_seql PUBLIC src/cantordb)

# --- Target 3: Interactive shell ---
add_executable(cantordb-shell
	src/cantordb/main.cpp
)
target_link_libraries(cantordb-shell PRIVATE cantordb_seql)
