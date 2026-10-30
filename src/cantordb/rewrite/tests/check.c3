module check;
import std::io;
import cantordb;

const int N = 1500;
const int MAXN = 1700;

uint rng = 12345;
fn uint rnd(uint m)
{
	rng = rng * 1664525u + 1013904223u;
	return (rng >> 8) % m;
}

// shadow model
bool[MAXN] alive;
bool[MAXN] has_i;
bool[MAXN] has_l;
bool[MAXN] has_d;
bool[MAXN] has_s;
int[MAXN] vi;
long[MAXN] vl;
double[MAXN] vd;
int[MAXN] vs;      // string is "v<vs>"
int total;
int failures;

fn String ename(int i) => string::tformat("e%d", i);
fn String sval(int v) => string::tformat("v%d", v);

fn void put(CantorDB* db, int i, bool fresh)
{
	vi[i] = (int)rnd(40) - 10;
	vl[i] = (long)rnd(30) * 3000000000L - 5;
	vd[i] = (double)rnd(25) / 4.0;
	vs[i] = (int)rnd(12);
	if (rnd(5) > 0) { has_i[i] = true; db.add_property_value("i", cantordb::pv_int(vi[i]), ename(i)); }
	if (rnd(5) > 0) { has_l[i] = true; db.add_property_value("l", cantordb::pv_long(vl[i]), ename(i)); }
	if (rnd(5) > 0) { has_d[i] = true; db.add_property_value("d", cantordb::pv_double(vd[i]), ename(i)); }
	if (rnd(5) > 0) { has_s[i] = true; db.add_property_value("s", cantordb::pv_string(sval(vs[i])), ename(i)); }
}

fn bool cmp(CmpOp op, double a, double b) => op == GREATER ? a > b : (op == LESSER ? a < b : a == b);

fn void verify(CantorDB* db, String label)
{
	for (int kind = 0; kind < 4; kind++)
	{
		for (int opn = 0; opn < 3; opn++)
		{
			if (kind == 3 && opn != 2) continue;   // strings: EQUAL only
			CmpOp op = opn == 0 ? GREATER : (opn == 1 ? LESSER : EQUAL);
			for (int probe = 0; probe < 4; probe++)
			{
				db.clear_cache();
				cantordb::PropertyValue pv;
				int ti = (int)rnd(40) - 10;
				long tl = (long)rnd(30) * 3000000000L - 5;
				double td = (double)rnd(25) / 4.0;
				int ts = (int)rnd(13);
				cantordb::Set* r;
				switch (kind)
				{
					case 0: r = db.where_elements(op == GREATER ? "T" : "T", "i", op, cantordb::pv_int(ti));
					case 1: r = db.where_elements("T", "l", op, cantordb::pv_long(tl));
					case 2: r = db.where_elements("T", "d", op, cantordb::pv_double(td));
					default: r = db.where_elements("T", "s", op, cantordb::pv_string(sval(ts)));
				}
				int expected = 0;
				int mismatches = 0;
				for (int i = 0; i < total; i++)
				{
					bool want = false;
					if (alive[i])
					{
						switch (kind)
						{
							case 0: want = has_i[i] && cmp(op, (double)vi[i], (double)ti);
							case 1: want = has_l[i] && (op == GREATER ? vl[i] > tl : (op == LESSER ? vl[i] < tl : vl[i] == tl));
							case 2: want = has_d[i] && cmp(op, vd[i], td);
							default: want = has_s[i] && vs[i] == ts;
						}
					}
					if (want) expected++;
					bool got = r ? db.is_element(ename(i), r.set_name) : false;
					if (got != want) mismatches++;
				}
				if (!r && expected > 0) mismatches++;
				if (mismatches > 0 || (r && db.get_cardinality(r.set_name) != expected))
				{
					failures++;
					io::printfn("FAIL %s kind=%d op=%d probe=%d expected=%d mismatches=%d card=%d", label, kind, opn, probe, expected, mismatches, r ? db.get_cardinality(r.set_name) : -1);
				}
			}
		}
	}
}

fn int main()
{
	CantorDB* db = cantordb::new_cantordb("check");
	db.create_property("i", "int");
	db.create_property("l", "long");
	db.create_property("d", "double");
	db.create_property("s", "string");
	db.create_set("T");
	total = N;
	for (int i = 0; i < N; i++)
	{
		db.create_set(ename(i));
		db.add_member("T", ename(i));
		alive[i] = true;
		put(db, i, true);
	}
	verify(db, "initial");

	for (int round = 1; round <= 6; round++)
	{
		// updates
		for (int n = 0; n < 200; n++)
		{
			int i = (int)rnd(total);
			if (!alive[i]) continue;
			int which = (int)rnd(4);
			if (which == 0 && has_i[i]) { vi[i] = (int)rnd(40) - 10; db.update_property(ename(i), "i", cantordb::pv_int(vi[i])); }
			if (which == 1 && has_l[i]) { vl[i] = (long)rnd(30) * 3000000000L - 5; db.update_property(ename(i), "l", cantordb::pv_long(vl[i])); }
			if (which == 2 && has_d[i]) { vd[i] = (double)rnd(25) / 4.0; db.update_property(ename(i), "d", cantordb::pv_double(vd[i])); }
			if (which == 3 && has_s[i]) { vs[i] = (int)rnd(12); db.update_property(ename(i), "s", cantordb::pv_string(sval(vs[i]))); }
		}
		// property deletes
		for (int n = 0; n < 60; n++)
		{
			int i = (int)rnd(total);
			if (!alive[i]) continue;
			int which = (int)rnd(4);
			if (which == 0 && has_i[i]) { has_i[i] = false; db.delete_property(ename(i), "i"); }
			if (which == 1 && has_l[i]) { has_l[i] = false; db.delete_property(ename(i), "l"); }
			if (which == 2 && has_d[i]) { has_d[i] = false; db.delete_property(ename(i), "d"); }
			if (which == 3 && has_s[i]) { has_s[i] = false; db.delete_property(ename(i), "s"); }
		}
		verify(db, "after updates/prop deletes");

		// hard deletes (ids get recycled by the creates below)
		for (int n = 0; n < 80; n++)
		{
			int i = (int)rnd(total);
			if (!alive[i]) continue;
			alive[i] = false;
			db.delete_set(ename(i), false);
		}
		verify(db, "after hard deletes");
		for (int n = 0; n < 60 && total < MAXN; n++)
		{
			int i = total++;
			db.create_set(ename(i));
			db.add_member("T", ename(i));
			alive[i] = true;
			has_i[i] = has_l[i] = has_d[i] = has_s[i] = false;
			put(db, i, true);
		}
		verify(db, "after recycled-id creates");

		// soft delete + restore keeps everything
		for (int n = 0; n < 40; n++)
		{
			int i = (int)rnd(total);
			if (!alive[i]) continue;
			db.delete_set(ename(i));
			db.restore_set(ename(i));
			db.add_member("T", ename(i));   // restore re-links to former parents; ignore "already added"
		}
		verify(db, "after soft delete + restore");
		if (round == 3)
		{
			// refill what hard deletes removed from the total so later rounds keep churning
			total = total;
		}
	}

	// back-link behaviour for computed results
	CantorDB* b = cantordb::new_cantordb("links");
	b.create_set("A"); b.create_set("B"); b.create_set("X"); b.create_set("Y");
	b.add_member("A", "X"); b.add_member("B", "X");
	cantordb::Set* inter = b.set_intersection("A", "B");
	String name = inter.set_name.copy(mem);
	String before = b.list_sets("X");
	bool listed_before = before.contains(name);
	b.free_string(before);
	b.add_member(name, "Y");                       // editing the result by hand creates the back-links
	String mid = b.list_sets("X");
	bool listed_mid = mid.contains(name);
	b.free_string(mid);
	String midy = b.list_sets("Y");
	bool y_listed = midy.contains(name);
	b.free_string(midy);
	b.clear_cache();
	String after = b.list_sets("X");
	bool listed_after = after.contains(name);
	b.free_string(after);
	String aftery = b.list_sets("Y");
	bool y_after = aftery.contains(name);
	b.free_string(aftery);
	io::printfn("backlinks: X before=%s (want false)  X after edit=%s (want true)  Y after edit=%s (want true)  X after clear=%s Y after clear=%s (want false false)",
		listed_before, listed_mid, y_listed, listed_after, y_after);
	if (listed_before || !listed_mid || !y_listed || listed_after || y_after) failures++;

	io::printfn("failures=%d", failures);
	return failures == 0 ? 0 : 1;
}
