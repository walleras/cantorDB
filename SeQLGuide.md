# SeQL guide

## Introduction

SeQL (Pronounced SehQL) is a set query language designed to manipulate and query databases, at this point only CantorDB, which are based upon naive set theory and set algebra.

### Quick examples

```
GET ELEMENTS OF Animals
GET SETS OF Animals
GET ALL SETS
GET CACHE SETS
GET CARDINALITY OF Animals
GET PROPERTIES OF Dog
IS Dog ELEMENT OF Animals
IS Animals SET OF Dog
IS Mammals SUBSET OF Animals
IS Animals SUPERSET OF Mammals
IS Mammals PROPER SUBSET OF Animals
IS Animals EQUAL Mammals
IS Animals DISJOINT Rocks
GET ELEMENTS OF Animals UNION Mammals
GET ELEMENTS OF Animals .DIFFERENCE. Mammals UNION Cats
GET ELEMENTS OF Animals WHERE legs > 0
CREATE SETS Bird Lizard
ADD Dog TO Animals
ADD PROPERTY speed = 5 TO Dog
REMOVE Dog FROM Animals
REMOVE PROPERTY speed FROM Dog
TRASH SETS Bird
DELETE SETS Lizard
```

### A note on philosophy

When I set out to design this language my ideal was to create a beautiful, easy to learn, and functional query language in that order. As such my design choices and rules were influenced by these principles. For example, never have two words when you can have one. Every word must justify its existence through either parser spacing or functionality. For GET ELEMENTS OF \<Set\> every word has said function. GET means that you are getting a list rather than just checking a boolean. ELEMENTS says which direction you are looking. And OF provides a convenient marker for the set algebra section.

The beauty of a query language is in its terse near-English vocabulary. A query language is almost like a caveman grunting what he wants. I chose this not because I think that non-programmers will be able to use my query language, SQL proved that isn't true, but because the idea of a query language as a terse near-English representation of operations is beautiful in and of itself. A query has structure and flow. SQL doesn't need FROM, SELECT name->students would work just as well to divide the statement, but SELECT name FROM students is more beautiful. It gives the query a rhythm of its own. SELECT name FROM students WHERE age > 18. This statement has structure and rhythm. Everything goes where it should, no surprises. I suppose that I am struggling to express just what exactly is so beautiful about a query language, but I hope that I have captured it with SeQL.

One perhaps controversial design decision is that I have decided to not use parentheses. Instead, inspired by the masterpiece that is the Principia Mathematica, I used dot notation. This will be explained in detail below. The reason I did this was beauty and functionality. Parentheses are ugly and destroy the purity of the query language. While a normal C-style program is full of symbols and the like that allow parentheses to match the flow of a program, a query language such as SQL usually only has parentheses as a symbol that is above and below the line. As such it captures the eye and interrupts scanning a query. In addition it is separated from what exactly is being promoted within the execution order. ((a UNION b) INTERSECTION c) has its first parenthesis a full, not counting the other parenthesis, three words before the operator that it is promoting. Depending on the query this can get rather ridiculous with the spacing between the operator and the parenthesis. This destroys the rhythm of a query language. Every other part of a query language either sits directly adjacent to what it is modifying or sets off a new part of the query. However, parentheses are different. They can be very distant from what they are modifying.

Dot notation is different. The dots sit directly next to the operator they are modifying. a ..UNION.. b .INTERSECTION. c has the precedence for INTERSECTION directly adjacent to it. A person does not have to hunt for parentheses or move their eyes back and forth just to understand what a specific operator does. I do have one negative to note, and that is when you are doing a complicated query the number of dots can be confusing as to ensuring that both sides are symmetrical. However, parentheses have a similar issue, with paper math often using brackets as a way of breaking up parentheses to ensure more readability. Code does not have this luxury. Thus, at least in my opinion, and I value my own opinion highly, despite its negatives, dot notation is the superior choice for query languages.

## Language dictionary

### Headers

**GET** — Begins a retrieval query. Signals that the query will return a list of results. Must be followed by a query type keyword (ELEMENTS, SETS, ALL, or CACHE).

**IS** — Begins a boolean query. Signals that the query will return Yes or No. Must be followed by a set name.

### Query types

**ELEMENTS** — Specifies that the query operates on the elements (children) of a set. Used after GET to retrieve a list of elements, or after IS to check element membership.

**SETS** — Used after GET to retrieve what sets a given set belongs to (its parents). Used after IS to check whether a set contains another as an element (inverse of ELEMENT).

**ALL** — Modifier used before SETS. GET ALL SETS returns every set in the database.

**CACHE** — Modifier used before SETS. GET CACHE SETS returns all cached (intermediate result) sets in the database.

### Relationship keywords

**ELEMENT** — Used in IS queries. Checks whether the first set is an element of the second set.

**SET** — Used in IS queries after a set name. Checks whether the second set is an element of the first set (inverse of ELEMENT).

**SUBSET** — Used in IS queries. Checks whether every element of the first set is also an element of the second set.

**SUPERSET** — Used in IS queries. Checks whether the first set contains every element of the second set.

**DISJOINT** — Used in IS queries. Checks whether two sets share no elements in common.

**PROPER** — Modifier used before SUBSET. IS A PROPER SUBSET OF B checks that A is a subset of B but not equal to B.

**EQUAL** — Used in IS queries. Checks whether two sets contain exactly the same elements.

### Connective

**OF** — Structural keyword that separates the query type from the set algebra section. Every word must justify its existence; OF provides the bridge between what you are asking for and where you are looking.

**TO** — Structural keyword used in ADD queries to specify the target set. ADD Dog TO Animals.

**FROM** — Structural keyword used in REMOVE queries to specify the source set. REMOVE Dog FROM Animals.

### Query types (continued)

**CARDINALITY** — Used after GET. GET CARDINALITY OF \<Set\> returns the number of elements in a set. Works with WHERE and set algebra collapse.

**PROPERTIES** — Used after GET. GET PROPERTIES OF \<Set\> returns the properties of a set.

### CRUD operations

**CREATE** — Creates new sets. Must be followed by SETS and one or more set names.

**TRASH** — Soft-deletes sets (moves to trash). Must be followed by SETS and one or more set names.

**DELETE** — Hard-deletes sets permanently. Must be followed by SETS and one or more set names.

**ADD** — Adds an element to a set or a property to a set. ADD \<elem\> TO \<Set\> for membership, ADD PROPERTY \<key\> TO \<Set\> for properties.

**REMOVE** — Removes an element from a set or a property from a set. REMOVE \<elem\> FROM \<Set\> for membership, REMOVE PROPERTY \<key\> FROM \<Set\> for properties.

**PROPERTY** — Modifier keyword used with ADD and REMOVE to indicate property operations rather than membership operations.

### Filtering

**WHERE** — Filters elements by property value. Placed after a set name. Supports >, <, =, >=, <= for numeric types and = for string and bool types. WHERE is resolved before set algebra, so each side of a UNION/INTERSECTION can have its own WHERE clause.

### Set algebra operators

**UNION** — Binary operator. Returns a new set containing all elements that are in either operand. Default priority: 0.

**INTERSECTION** — Binary operator. Returns a new set containing only elements that are in both operands. Default priority: 0.

**DIFFERENCE** — Binary operator. Returns a new set containing elements in the left operand that are not in the right operand. Default priority: 0.

**SYMDIFF** — Binary operator. Symmetric difference. Returns a new set containing elements that are in either operand but not in both. Default priority: 0.

### Dot notation (precedence)

All set algebra operators have the same default priority (0). To control evaluation order, wrap an operator in dots. The number of dots on each side must be equal. More dots means higher priority (evaluated first).

- `.UNION.` — priority 1
- `..UNION..` — priority 2
- `...UNION...` — priority 3

This applies to all set albegra operators (UNION, INTERSECTION, DIFFERENCE, SYMDIFF). Dots around anything else is discarded.

## Query snippets

`GET ELEMENTS OF <Set>` — Lists all elements of a set.

`GET SETS OF <Set>` — Lists all sets that a set belongs to.

`GET ALL SETS` — Lists every set in the database.

`GET CACHE SETS` — Lists all cached intermediate result sets.

`IS <Set> ELEMENT OF <Set>` — Checks if the first set is an element of the second.

`IS <Set> SET OF <Set>` — Checks if the first set contains the second as an element.

`IS <Set> SUBSET OF <Set>` — Checks if every element of the first set is also in the second.

`IS <Set> SUPERSET OF <Set>` — Checks if the first set contains every element of the second.

`GET ELEMENTS OF <Set> UNION <Set>` — Lists elements in either set.

`GET ELEMENTS OF <Set> INTERSECTION <Set>` — Lists elements in both sets.

`GET ELEMENTS OF <Set> DIFFERENCE <Set>` — Lists elements in the first set but not the second.

`GET ELEMENTS OF <Set> SYMDIFF <Set>` — Lists elements in either set but not both.

`GET ELEMENTS OF <Set> .UNION. <Set> INTERSECTION <Set>` — Union is evaluated first due to higher dot priority, then intersected with the third set.

`IS <Set> DISJOINT <Set>` — Checks if two sets share no elements.

`IS <Set> EQUAL <Set>` — Checks if two sets contain exactly the same elements.

`IS <Set> PROPER SUBSET OF <Set>` — Checks if the first set is a strict subset of the second (subset but not equal).

`GET CARDINALITY OF <Set>` — Returns the number of elements in a set.

`GET CARDINALITY OF <Set> WHERE <key> > <value>` — Returns the count of elements matching a filter.

`GET CARDINALITY OF <Set> UNION <Set>` — Returns the count of the union of two sets.

`GET PROPERTIES OF <Set>` — Lists all properties of a set.

`CREATE SETS <name> <name> ...` — Creates one or more new sets.

`TRASH SETS <name> <name> ...` — Soft-deletes one or more sets (recoverable).

`DELETE SETS <name> <name> ...` — Permanently deletes one or more sets.

`ADD <elem> TO <Set>` — Adds an element to a set.

`REMOVE <elem> FROM <Set>` — Removes an element from a set.

`ADD PROPERTY <key> TO <Set>` — Adds a property to a set, zero-initialized.

`ADD PROPERTY <key> = <value> TO <Set>` — Adds a property with a value. Type is inferred (number, string, or bool).

`REMOVE PROPERTY <key> FROM <Set>` — Removes a property from a set.

`GET ELEMENTS OF <Set> WHERE <key> > <value>` — Lists elements where property is greater than value.

`GET ELEMENTS OF <Set> WHERE <key> = <value>` — Lists elements where property equals value. Works for int, double, string, and bool.

`GET ELEMENTS OF <Set> WHERE <key> >= <value>` — Lists elements where property is greater than or equal to value.

`GET ELEMENTS OF <Set> WHERE <key> < <value>` — Lists elements where property is less than value.

`GET ELEMENTS OF <Set> WHERE <key> <= <value>` — Lists elements where property is less than or equal to value.

`GET ELEMENTS OF <Set> WHERE <key> > <value> UNION <Set> WHERE <key> > <value>` — WHERE clauses apply per-set before the UNION.