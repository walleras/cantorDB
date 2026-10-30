module bench;

import cantordb;
import std::io;
import std::time;

fn uint hsh(uint i, uint k)
{
	uint x = i * 2654435761u + k * 97u;
	x ^= x >> 15;
	x *= 2246822519u;
	x ^= x >> 13;
	return x;
}

fn double ms_since(Clock c) => (double)(long)c.to_now() / 1000000.0;

const int R = 20;

macro @run(CantorDB* db, String name, #expr)
{
	double op = 0;
	double clr = 0;
	cantordb::Set* s;
	for (int r = 0; r < R; r++)
	{
		Clock a = clock::now();
		db.clear_cache();
		clr += ms_since(a);
		a = clock::now();
		s = #expr;
		op += ms_since(a);
	}
	int card = s ? db.get_cardinality(s.set_name) : -1;
	io::printfn("%-17s op %9.3f ms   clear_cache %9.3f ms   card=%d", name, op / R, clr / R, card);
}

fn int main(String[] args)
{
	uint n = 100000;
	if (args.len > 1) n = (uint)args[1].to_int()!!;
	uint tags = 8;
	cantordb::CantorDB* db = cantordb::new_cantordb("bench");
	Clock c;

	c = clock::now();
	for (uint k = 0; k < tags; k++) { @pool() { db.create_set(string::tformat("T%d", k)); }; }
	for (uint i = 0; i < n; i++) { @pool() { db.create_set(string::tformat("e%d", i)); }; }
	io::printfn("build_sets        %10.2f ms", ms_since(c));

	c = clock::now();
	for (uint i = 0; i < n; i++)
	{
		@pool()
		{
			String e = string::tformat("e%d", i);
			for (uint k = 0; k < tags; k++)
			{
				if (hsh(i, k) % (uint)(k + 2) == 0) db.add_member(string::tformat("T%d", k), e);
			}
		};
	}
	io::printfn("build_members     %10.2f ms", ms_since(c));

	c = clock::now();
	db.create_property("score", "int");
	for (uint i = 0; i < n; i++)
	{
		@pool() { db.add_property_value("score", cantordb::pv_int((int)(hsh(i, 99) % 1000)), string::tformat("e%d", i)); };
	}
	io::printfn("build_props       %10.2f ms", ms_since(c));

	@run(db, "intersection", db.set_intersection("T0", "T1"));
	@run(db, "union", db.set_union("T0", "T2"));
	@run(db, "difference", db.set_difference("T0", "T1"));
	@run(db, "symdiff", db.symmetric_difference("T1", "T2"));
	c = clock::now();
	db.where_elements_greater_than("T0", "score", cantordb::pv_int(500));
	io::printfn("where_gt COLD (builds the index)  %9.3f ms", ms_since(c));
	@run(db, "where_gt", db.where_elements_greater_than("T0", "score", cantordb::pv_int(500)));
	@run(db, "where_eq", db.where_elements_equal_than("T0", "score", cantordb::pv_int(7)));

	c = clock::now();
	long hits = 0;
	for (uint i = 0; i < n; i++)
	{
		@pool() { if (db.is_element(string::tformat("e%d", i), "T0")) hits++; };
	}
	io::printfn("%-17s %10.2f ms total    hits=%d", "is_element x N", ms_since(c), hits);
	return 0;
}
