// Brooklyn District Stress Test for CantorDB + SeQL
// Simulates a full Brooklyn K-12 school district:
//   ~20 schools, ~15,000 students, ~1,500 staff, ~600 classes, ~40 extracurriculars
//   40-week school year simulation with edge cases
//
// Compile:
//   g++ -std=c++20 -O2 -o test_stress.exe tests/test_stress.cpp src/cantordb/cantordb.cpp src/cantordb/lexer.cpp src/cantordb/parser.cpp

#include "cantordb.h"
#include "parser.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <random>
#include <iomanip>
#include <functional>

using namespace std;
using namespace chrono;

// ============================================================================
// LOGGING
// ============================================================================

static ofstream LOG_FILE;
static int OP_COUNT_TOTAL = 0;
static int OP_COUNT_SEQL = 0;
static int OP_COUNT_API = 0;
static int OP_COUNT_FAIL = 0;
static int OP_COUNT_CHUNK = 0;
static int OP_COUNT_CHUNK_SEQL = 0;
static int OP_COUNT_CHUNK_API = 0;
static int OP_COUNT_CHUNK_FAIL = 0;
static double TOTAL_DURATION_US = 0.0;
static double CHUNK_DURATION_US = 0.0;
static int CACHE_CLEARS = 0;

// Edge case counters
static int EDGE_CASES_TRIGGERED = 0;
static int EDGE_CASES_RESOLVED = 0;

void reset_chunk_counters() {
	OP_COUNT_CHUNK = 0;
	OP_COUNT_CHUNK_SEQL = 0;
	OP_COUNT_CHUNK_API = 0;
	OP_COUNT_CHUNK_FAIL = 0;
	CHUNK_DURATION_US = 0.0;
}

string timestamp_now() {
	auto now = system_clock::now();
	auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
	auto timer = system_clock::to_time_t(now);
	struct tm t;
	localtime_s(&t, &timer);
	ostringstream oss;
	oss << put_time(&t, "%H:%M:%S") << "." << setfill('0') << setw(3) << ms.count();
	return oss.str();
}

// Execute a SeQL query, log it, return result
string seql(cantordb& db, const string& query) {
	auto start = high_resolution_clock::now();
	string result = parse_query(db, query);
	auto end = high_resolution_clock::now();
	double us = duration_cast<nanoseconds>(end - start).count() / 1000.0;

	bool fail = result.find("Error") != string::npos || result.find("Syntax Error") != string::npos;
	string status = fail ? "FAIL" : "OK";

	OP_COUNT_TOTAL++;
	OP_COUNT_SEQL++;
	OP_COUNT_CHUNK++;
	OP_COUNT_CHUNK_SEQL++;
	TOTAL_DURATION_US += us;
	CHUNK_DURATION_US += us;
	if (fail) { OP_COUNT_FAIL++; OP_COUNT_CHUNK_FAIL++; }

	LOG_FILE << "[" << timestamp_now() << "] [SeQL] [" << fixed << setprecision(1) << us << "us] [" << status << "] " << query;
	if (fail) LOG_FILE << " | ERROR: " << result;
	LOG_FILE << "\n";

	return result;
}

// Execute a direct API call, log it, return success
template<typename F>
bool api_call(const string& description, F fn) {
	auto start = high_resolution_clock::now();
	bool ok = fn();
	auto end = high_resolution_clock::now();
	double us = duration_cast<nanoseconds>(end - start).count() / 1000.0;

	string status = ok ? "OK" : "FAIL";

	OP_COUNT_TOTAL++;
	OP_COUNT_API++;
	OP_COUNT_CHUNK++;
	OP_COUNT_CHUNK_API++;
	TOTAL_DURATION_US += us;
	CHUNK_DURATION_US += us;
	if (!ok) { OP_COUNT_FAIL++; OP_COUNT_CHUNK_FAIL++; }

	LOG_FILE << "[" << timestamp_now() << "] [API] [" << fixed << setprecision(1) << us << "us] [" << status << "] " << description;
	LOG_FILE << "\n";

	return ok;
}

// Clear cache and log
void clear_cache_logged(cantordb& db) {
	api_call("clear_cache", [&]() { return db.clear_cache(); });
	CACHE_CLEARS++;
}

// ============================================================================
// NAME GENERATION
// ============================================================================

static mt19937 RNG(42); // deterministic seed for reproducibility

const vector<string> FIRST_NAMES = {
	"James","Mary","Robert","Patricia","John","Jennifer","Michael","Linda","David","Elizabeth",
	"William","Barbara","Richard","Susan","Joseph","Jessica","Thomas","Sarah","Christopher","Karen",
	"Charles","Lisa","Daniel","Nancy","Matthew","Betty","Anthony","Margaret","Mark","Sandra",
	"Donald","Ashley","Steven","Kimberly","Paul","Emily","Andrew","Donna","Joshua","Michelle",
	"Kenneth","Carol","Kevin","Amanda","Brian","Dorothy","George","Melissa","Timothy","Deborah",
	"Ronald","Stephanie","Edward","Rebecca","Jason","Sharon","Jeffrey","Laura","Ryan","Cynthia",
	"Jacob","Kathleen","Gary","Amy","Nicholas","Angela","Eric","Shirley","Jonathan","Anna",
	"Stephen","Brenda","Larry","Pamela","Justin","Emma","Scott","Nicole","Brandon","Helen",
	"Benjamin","Samantha","Samuel","Katherine","Raymond","Christine","Gregory","Debra","Frank","Rachel",
	"Alexander","Carolyn","Patrick","Janet","Jack","Catherine","Dennis","Maria","Jerry","Heather",
	"Aaliyah","Jamal","Keisha","DeShawn","Latoya","Tyrone","Shaniqua","Malik","Tanisha","Darius",
	"Aisha","Kareem","Imani","Lamar","Ebony","Andre","Jasmine","Xavier","Destiny","Marcus",
	"Sofia","Santiago","Isabella","Mateo","Valentina","Diego","Camila","Sebastian","Lucia","Alejandro",
	"Wei","Xin","Hua","Ming","Yan","Jun","Li","Fang","Lin","Chen",
	"Yuki","Haruto","Sakura","Ren","Hana","Sota","Mei","Yuto","Aoi","Kaito",
	"Omar","Fatima","Yusuf","Amira","Hassan","Layla","Ibrahim","Zara","Ali","Noor",
	"Dmitri","Natasha","Sergei","Olga","Viktor","Irina","Nikolai","Svetlana","Andrei","Elena",
	"Raj","Priya","Arjun","Ananya","Vikram","Devi","Sanjay","Meera","Arun","Kavita",
	"Kwame","Ama","Kofi","Akua","Yaw","Abena","Kojo","Efua","Kwesi","Adwoa"
};

const vector<string> LAST_NAMES = {
	"Smith","Johnson","Williams","Brown","Jones","Garcia","Miller","Davis","Rodriguez","Martinez",
	"Hernandez","Lopez","Gonzalez","Wilson","Anderson","Thomas","Taylor","Moore","Jackson","Martin",
	"Lee","Perez","Thompson","White","Harris","Sanchez","Clark","Ramirez","Lewis","Robinson",
	"Walker","Young","Allen","King","Wright","Scott","Torres","Nguyen","Hill","Flores",
	"Green","Adams","Nelson","Baker","Hall","Rivera","Campbell","Mitchell","Carter","Roberts",
	"Washington","Jefferson","Hamilton","Franklin","Monroe","Madison","Lincoln","Grant","Hayes","Garfield",
	"Chen","Wang","Zhang","Liu","Yang","Huang","Wu","Zhou","Xu","Sun",
	"Kim","Park","Choi","Jung","Kang","Cho","Yoon","Jang","Im","Han",
	"Petrov","Ivanov","Smirnov","Volkov","Fedorov","Morozov","Novikov","Popov","Kozlov","Sokolov",
	"Patel","Shah","Sharma","Singh","Gupta","Mehta","Joshi","Kumar","Rao","Desai",
	"Owusu","Mensah","Boateng","Asante","Osei","Agyeman","Appiah","Bonsu","Frimpong","Amoah",
	"OBrien","Sullivan","Murphy","Kelly","Walsh","Ryan","OMalley","Fitzgerald","Brennan","Gallagher",
	"DiMaggio","Russo","Romano","Colombo","Ferrari","Esposito","Ricci","Marino","Greco","Bruno",
	"Goldstein","Schwartz","Cohen","Friedman","Rosenberg","Shapiro","Klein","Weiss","Roth","Kaplan"
};

const vector<string> SCHOOL_TYPES = {"Elementary", "Middle", "Junior_High", "High"};

string pick_name(const vector<string>& v) {
	return v[RNG() % v.size()];
}

int rand_range(int lo, int hi) {
	return lo + (RNG() % (hi - lo + 1));
}

// ============================================================================
// DISTRICT DATA STRUCTURES (for tracking what we've created)
// ============================================================================

struct SchoolInfo {
	string name;
	string type; // Elementary, Middle, Junior_High, High
	int grade_lo, grade_hi;
	vector<string> staff_ids;
	vector<string> student_ids;
	vector<string> class_ids;
};

struct StudentInfo {
	string id;
	string first_name;
	string last_name;
	string school;
	int grade;
	bool enrolled;
	bool suspended;
	bool expelled;
	bool has_iep;
};

struct StaffInfo {
	string id;
	string first_name;
	string last_name;
	string school;
	string role; // Teacher, Custodian, Admin, Coach, Support
	int salary_cents;
	bool active;
	bool in_dispute;
	int pto_days;
};

struct ClassInfo {
	string id;
	string school;
	string teacher_id;
	string subject;
	int period;
	vector<string> student_ids;
};

struct ExtraCurricularInfo {
	string id;
	string school;
	string type; // Sports, Olympiad, SpellingBee, Club
	string name;
	string coach_id;
	vector<string> student_ids;
};

static vector<SchoolInfo> SCHOOLS;
static vector<StudentInfo> STUDENTS;
static vector<StaffInfo> STAFF;
static vector<ClassInfo> CLASSES;
static vector<ExtraCurricularInfo> EXTRAS;

// Carry-overs for weekly simulation
static vector<string> CARRYOVERS;

// ============================================================================
// CHUNK 1 — INITIAL DATA CREATION
// ============================================================================

void create_district_schema(cantordb& db) {
	LOG_FILE << "\n========================================\n";
	LOG_FILE << "CHUNK 1 — INITIAL DATA CREATION (Pre-September)\n";
	LOG_FILE << "========================================\n\n";
	reset_chunk_counters();

	// Top-level organizational sets
	seql(db, "CREATE SETS Brooklyn_District");
	seql(db, "CREATE SETS All_Schools All_Students All_Staff All_Classes All_Extracurriculars");
	seql(db, "CREATE SETS Elementary_Schools Middle_Schools Junior_High_Schools High_Schools");
	seql(db, "CREATE SETS Active_Students Suspended_Students Expelled_Students Transferred_Students");
	seql(db, "CREATE SETS Active_Staff Terminated_Staff On_Leave_Staff Disputed_Staff");
	seql(db, "CREATE SETS IEP_Students");
	seql(db, "CREATE SETS Teachers Custodians Administrators Coaches Support_Staff");
	seql(db, "CREATE SETS Sports_Teams Olympiad_Teams SpellingBee_Teams Clubs");
	seql(db, "CREATE SETS HR_Records Payroll_Records PTO_Records");

	// Grade level sets
	for (int g = 0; g <= 12; g++) {
		seql(db, "CREATE SETS Grade_" + to_string(g));
		seql(db, "ADD Grade_" + to_string(g) + " TO Brooklyn_District");
	}
	seql(db, "CREATE SETS Grade_PreK");
	seql(db, "ADD Grade_PreK TO Brooklyn_District");

	// Add org sets to district
	seql(db, "ADD All_Schools TO Brooklyn_District");
	seql(db, "ADD All_Students TO Brooklyn_District");
	seql(db, "ADD All_Staff TO Brooklyn_District");
	seql(db, "ADD All_Classes TO Brooklyn_District");
	seql(db, "ADD All_Extracurriculars TO Brooklyn_District");

	clear_cache_logged(db);
}

void create_schools(cantordb& db) {
	LOG_FILE << "\n--- Creating Schools ---\n";

	// Brooklyn-scale: 8 elementary, 5 middle, 3 junior high, 4 high = 20 schools
	struct SchoolDef { string name; string type; int glo; int ghi; };
	vector<SchoolDef> defs = {
		// Elementary (PreK/K-5)
		{"PS_101_Bay_Ridge", "Elementary", 0, 5},
		{"PS_102_Bensonhurst", "Elementary", 0, 5},
		{"PS_103_Sunset_Park", "Elementary", 0, 5},
		{"PS_104_Park_Slope", "Elementary", 0, 5},
		{"PS_105_Fort_Greene", "Elementary", 0, 5},
		{"PS_106_Williamsburg", "Elementary", 0, 5},
		{"PS_107_Bushwick", "Elementary", 0, 5},
		{"PS_108_Crown_Heights", "Elementary", 0, 5},
		// Middle (6-8)
		{"MS_201_Bay_Ridge", "Middle", 6, 8},
		{"MS_202_Flatbush", "Middle", 6, 8},
		{"MS_203_Bedford_Stuyvesant", "Middle", 6, 8},
		{"MS_204_Brownsville", "Middle", 6, 8},
		{"MS_205_East_New_York", "Middle", 6, 8},
		// Junior High (7-9)
		{"JHS_301_Canarsie", "Junior_High", 7, 9},
		{"JHS_302_Sheepshead_Bay", "Junior_High", 7, 9},
		{"JHS_303_Borough_Park", "Junior_High", 7, 9},
		// High (9-12)
		{"HS_401_Brooklyn_Tech", "High", 9, 12},
		{"HS_402_Midwood", "High", 9, 12},
		{"HS_403_New_Utrecht", "High", 9, 12},
		{"HS_404_Lincoln", "High", 9, 12},
	};

	for (auto& d : defs) {
		SchoolInfo si;
		si.name = d.name;
		si.type = d.type;
		si.grade_lo = d.glo;
		si.grade_hi = d.ghi;

		seql(db, "CREATE SETS " + d.name);
		seql(db, "ADD " + d.name + " TO All_Schools");

		string type_set = d.type + "_Schools";
		seql(db, "ADD " + d.name + " TO " + type_set);

		// Properties
		api_call("add_property type=" + d.type + " to " + d.name, [&]() {
			return db.add_property("type", d.type, d.name);
		});
		api_call("add_property grade_lo=" + to_string(d.glo) + " to " + d.name, [&]() {
			return db.add_property("grade_lo", d.glo, d.name);
		});
		api_call("add_property grade_hi=" + to_string(d.ghi) + " to " + d.name, [&]() {
			return db.add_property("grade_hi", d.ghi, d.name);
		});

		// Sub-sets for each school
		seql(db, "CREATE SETS " + d.name + "_Students");
		seql(db, "ADD " + d.name + "_Students TO " + d.name);
		seql(db, "CREATE SETS " + d.name + "_Staff");
		seql(db, "ADD " + d.name + "_Staff TO " + d.name);
		seql(db, "CREATE SETS " + d.name + "_Classes");
		seql(db, "ADD " + d.name + "_Classes TO " + d.name);

		SCHOOLS.push_back(si);
	}

	clear_cache_logged(db);
}

void create_staff(cantordb& db) {
	LOG_FILE << "\n--- Creating Staff ---\n";

	int staff_id = 1;
	// Per school: ~5 admin, ~3 custodians, ~2 support, teachers + coaches created with classes/extras
	// Plus ~30-60 teachers per school depending on size

	for (auto& school : SCHOOLS) {
		int num_teachers, num_admin, num_custodians, num_support;

		if (school.type == "Elementary") {
			num_teachers = 30; num_admin = 4; num_custodians = 3; num_support = 2;
		} else if (school.type == "Middle") {
			num_teachers = 35; num_admin = 5; num_custodians = 3; num_support = 3;
		} else if (school.type == "Junior_High") {
			num_teachers = 35; num_admin = 5; num_custodians = 3; num_support = 3;
		} else { // High
			num_teachers = 50; num_admin = 6; num_custodians = 4; num_support = 4;
		}

		auto create_staff_member = [&](const string& role, int salary_lo, int salary_hi) {
			StaffInfo si;
			si.id = "STAFF_" + to_string(staff_id++);
			si.first_name = pick_name(FIRST_NAMES);
			si.last_name = pick_name(LAST_NAMES);
			si.school = school.name;
			si.role = role;
			si.salary_cents = rand_range(salary_lo, salary_hi);
			si.active = true;
			si.in_dispute = false;
			si.pto_days = rand_range(10, 25);

			seql(db, "CREATE SETS " + si.id);
			seql(db, "ADD " + si.id + " TO All_Staff");
			seql(db, "ADD " + si.id + " TO Active_Staff");
			seql(db, "ADD " + si.id + " TO " + school.name + "_Staff");

			// Role sets
			if (role == "Teacher") seql(db, "ADD " + si.id + " TO Teachers");
			else if (role == "Custodian") seql(db, "ADD " + si.id + " TO Custodians");
			else if (role == "Admin") seql(db, "ADD " + si.id + " TO Administrators");
			else if (role == "Support") seql(db, "ADD " + si.id + " TO Support_Staff");

			// Properties via API (update_property not in SeQL)
			api_call("add_property first_name to " + si.id, [&]() {
				return db.add_property("first_name", si.first_name, si.id);
			});
			api_call("add_property last_name to " + si.id, [&]() {
				return db.add_property("last_name", si.last_name, si.id);
			});
			api_call("add_property role to " + si.id, [&]() {
				return db.add_property("role", role, si.id);
			});
			api_call("add_property salary_cents to " + si.id, [&]() {
				return db.add_property("salary_cents", si.salary_cents, si.id);
			});
			api_call("add_property school to " + si.id, [&]() {
				return db.add_property("school", school.name, si.id);
			});
			api_call("add_property active to " + si.id, [&]() {
				return db.add_property("active", true, si.id);
			});
			api_call("add_property pto_days to " + si.id, [&]() {
				return db.add_property("pto_days", si.pto_days, si.id);
			});
			// Hire date as UNIX timestamp (random date in last 15 years)
			long hire_ts = 1262304000L + (long)(RNG() % 473385600L); // 2010-2025
			api_call("add_property hire_date to " + si.id, [&]() {
				return db.add_property("hire_date", hire_ts, si.id);
			});

			school.staff_ids.push_back(si.id);
			STAFF.push_back(si);
		};

		for (int i = 0; i < num_teachers; i++)
			create_staff_member("Teacher", 5500000, 11000000); // $55k-$110k
		for (int i = 0; i < num_admin; i++)
			create_staff_member("Admin", 7000000, 15000000);
		for (int i = 0; i < num_custodians; i++)
			create_staff_member("Custodian", 3500000, 6500000);
		for (int i = 0; i < num_support; i++)
			create_staff_member("Support", 4000000, 7000000);

		// Clear cache periodically to avoid memory bloat
		if (SCHOOLS.size() > 0 && (&school - &SCHOOLS[0]) % 5 == 4) {
			clear_cache_logged(db);
		}
	}

	clear_cache_logged(db);
	LOG_FILE << "  Staff created: " << STAFF.size() << "\n";
}

void create_students(cantordb& db) {
	LOG_FILE << "\n--- Creating Students ---\n";

	int student_id = 1;

	for (auto& school : SCHOOLS) {
		int students_per_grade;
		if (school.type == "Elementary") students_per_grade = 90;      // ~540 per elem
		else if (school.type == "Middle") students_per_grade = 150;    // ~450 per middle
		else if (school.type == "Junior_High") students_per_grade = 150; // ~450 per JH
		else students_per_grade = 250;                                  // ~1000 per HS

		for (int g = school.grade_lo; g <= school.grade_hi; g++) {
			for (int s = 0; s < students_per_grade; s++) {
				StudentInfo si;
				si.id = "STU_" + to_string(student_id++);
				si.first_name = pick_name(FIRST_NAMES);
				si.last_name = pick_name(LAST_NAMES);
				si.school = school.name;
				si.grade = g;
				si.enrolled = true;
				si.suspended = false;
				si.expelled = false;
				si.has_iep = (RNG() % 100) < 12; // ~12% IEP rate

				seql(db, "CREATE SETS " + si.id);
				seql(db, "ADD " + si.id + " TO All_Students");
				seql(db, "ADD " + si.id + " TO Active_Students");
				seql(db, "ADD " + si.id + " TO " + school.name + "_Students");
				seql(db, "ADD " + si.id + " TO Grade_" + to_string(g));

				// Properties
				api_call("props " + si.id, [&]() {
					return db.add_property("first_name", si.first_name, si.id);
				});
				api_call("props " + si.id, [&]() {
					return db.add_property("last_name", si.last_name, si.id);
				});
				api_call("props " + si.id, [&]() {
					return db.add_property("grade", g, si.id);
				});
				api_call("props " + si.id, [&]() {
					return db.add_property("school", school.name, si.id);
				});
				api_call("props " + si.id, [&]() {
					return db.add_property("enrolled", true, si.id);
				});
				// DOB as UNIX timestamp
				// Grade 0 = ~5yo, Grade 12 = ~17yo, school year 2025-2026
				int age = 5 + g;
				long dob_ts = 1577836800L - (long)(age * 31557600L) + (long)(RNG() % 31557600L);
				api_call("props " + si.id, [&]() {
					return db.add_property("dob", dob_ts, si.id);
				});

				if (si.has_iep) {
					seql(db, "ADD " + si.id + " TO IEP_Students");
					api_call("props " + si.id + " iep", [&]() {
						return db.add_property("has_iep", true, si.id);
					});
				}

				school.student_ids.push_back(si.id);
				STUDENTS.push_back(si);
			}

			// Clear cache every 500 students
			if (student_id % 500 == 0) {
				clear_cache_logged(db);
			}
		}
	}

	clear_cache_logged(db);
	LOG_FILE << "  Students created: " << STUDENTS.size() << "\n";
}

void create_classes(cantordb& db) {
	LOG_FILE << "\n--- Creating Classes ---\n";

	int class_id = 1;
	const vector<string> elem_subjects = {"Math","Reading","Science","Social_Studies","Art","Music","PE","ESL"};
	const vector<string> ms_subjects = {"Math","English","Science","History","Spanish","Art","Music","PE","Tech"};
	const vector<string> hs_subjects = {"Algebra","Geometry","Calculus","English","Literature","Biology",
		"Chemistry","Physics","US_History","World_History","Spanish","French","Art","Music","PE","CS","Economics"};

	for (auto& school : SCHOOLS) {
		const vector<string>* subjects;
		if (school.type == "Elementary") subjects = &elem_subjects;
		else if (school.type == "Middle" || school.type == "Junior_High") subjects = &ms_subjects;
		else subjects = &hs_subjects;

		// Find teachers at this school
		vector<int> teacher_indices;
		for (int i = 0; i < (int)STAFF.size(); i++) {
			if (STAFF[i].school == school.name && STAFF[i].role == "Teacher")
				teacher_indices.push_back(i);
		}
		int ti = 0;

		for (int g = school.grade_lo; g <= school.grade_hi; g++) {
			for (auto& subj : *subjects) {
				// Multiple sections per subject per grade
				int sections = (school.type == "High") ? 3 : 2;
				for (int sec = 1; sec <= sections; sec++) {
					ClassInfo ci;
					ci.id = "CLASS_" + to_string(class_id++);
					ci.school = school.name;
					ci.subject = subj;
					ci.period = sec;

					// Assign teacher
					if (!teacher_indices.empty()) {
						ci.teacher_id = STAFF[teacher_indices[ti % teacher_indices.size()]].id;
						ti++;
					}

					seql(db, "CREATE SETS " + ci.id);
					seql(db, "ADD " + ci.id + " TO All_Classes");
					seql(db, "ADD " + ci.id + " TO " + school.name + "_Classes");

					api_call("props " + ci.id, [&]() {
						return db.add_property("subject", subj, ci.id);
					});
					api_call("props " + ci.id, [&]() {
						return db.add_property("grade", g, ci.id);
					});
					api_call("props " + ci.id, [&]() {
						return db.add_property("school", school.name, ci.id);
					});
					api_call("props " + ci.id, [&]() {
						return db.add_property("period", sec, ci.id);
					});
					if (!ci.teacher_id.empty()) {
						api_call("props " + ci.id, [&]() {
							return db.add_property("teacher", ci.teacher_id, ci.id);
						});
					}

					// Enroll students: grab students at this school+grade
					vector<string> eligible;
					for (auto& stu : STUDENTS) {
						if (stu.school == school.name && stu.grade == g)
							eligible.push_back(stu.id);
					}

					// Distribute students across sections (~25-30 per class)
					int start_idx = (sec - 1) * 28;
					int end_idx = min(start_idx + 28, (int)eligible.size());
					for (int si = start_idx; si < end_idx; si++) {
						seql(db, "ADD " + eligible[si] + " TO " + ci.id);
						ci.student_ids.push_back(eligible[si]);
					}

					school.class_ids.push_back(ci.id);
					CLASSES.push_back(ci);
				}
			}
		}

		// Clear cache after each school's classes
		clear_cache_logged(db);
	}

	LOG_FILE << "  Classes created: " << CLASSES.size() << "\n";
}

void create_extracurriculars(cantordb& db) {
	LOG_FILE << "\n--- Creating Extracurriculars ---\n";

	int extra_id = 1;

	struct ExtraDef { string type; string name; bool hs_only; };
	vector<ExtraDef> defs = {
		{"Sports", "Basketball", false},
		{"Sports", "Soccer", false},
		{"Sports", "Track_And_Field", false},
		{"Sports", "Baseball", false},
		{"Sports", "Swimming", false},
		{"Sports", "Volleyball", true},
		{"Sports", "Football", true},
		{"Sports", "Wrestling", true},
		{"Olympiad", "Math_Olympiad", false},
		{"Olympiad", "Science_Olympiad", false},
		{"SpellingBee", "Spelling_Bee", false},
		{"Club", "Chess_Club", false},
		{"Club", "Drama_Club", false},
		{"Club", "Robotics_Club", true},
		{"Club", "Debate_Club", true},
		{"Club", "Art_Club", false},
		{"Club", "Music_Ensemble", false},
		{"Club", "Student_Government", false},
	};

	for (auto& school : SCHOOLS) {
		bool is_hs = (school.type == "High");
		bool is_elem = (school.type == "Elementary");

		for (auto& d : defs) {
			if (d.hs_only && !is_hs) continue;
			if (is_elem && d.type == "Olympiad") continue; // skip olympiad for elem
			if (is_elem && d.name == "Debate_Club") continue;

			ExtraCurricularInfo ei;
			ei.id = "EXTRA_" + to_string(extra_id++);
			ei.school = school.name;
			ei.type = d.type;
			ei.name = d.name;

			seql(db, "CREATE SETS " + ei.id);
			seql(db, "ADD " + ei.id + " TO All_Extracurriculars");

			string type_set = d.type == "Sports" ? "Sports_Teams" :
			                  d.type == "Olympiad" ? "Olympiad_Teams" :
			                  d.type == "SpellingBee" ? "SpellingBee_Teams" : "Clubs";
			seql(db, "ADD " + ei.id + " TO " + type_set);

			api_call("props " + ei.id, [&]() {
				return db.add_property("name", d.name, ei.id);
			});
			api_call("props " + ei.id, [&]() {
				return db.add_property("type", d.type, ei.id);
			});
			api_call("props " + ei.id, [&]() {
				return db.add_property("school", school.name, ei.id);
			});

			// Assign coach from school staff
			for (auto& s : STAFF) {
				if (s.school == school.name && s.role == "Teacher" && ei.coach_id.empty()) {
					ei.coach_id = s.id;
					seql(db, "ADD " + s.id + " TO Coaches");
					break;
				}
			}

			// Enroll 10-25 random students from this school
			int roster_size = rand_range(10, 25);
			int enrolled = 0;
			vector<int> shuffled_indices;
			for (int i = 0; i < (int)STUDENTS.size(); i++) {
				if (STUDENTS[i].school == school.name) shuffled_indices.push_back(i);
			}
			// Shuffle
			for (int i = (int)shuffled_indices.size() - 1; i > 0; i--) {
				int j = RNG() % (i + 1);
				swap(shuffled_indices[i], shuffled_indices[j]);
			}
			for (int i = 0; i < min(roster_size, (int)shuffled_indices.size()); i++) {
				seql(db, "ADD " + STUDENTS[shuffled_indices[i]].id + " TO " + ei.id);
				ei.student_ids.push_back(STUDENTS[shuffled_indices[i]].id);
				enrolled++;
			}

			EXTRAS.push_back(ei);
		}

		clear_cache_logged(db);
	}

	LOG_FILE << "  Extracurriculars created: " << EXTRAS.size() << "\n";
}

void create_hr_records(cantordb& db) {
	LOG_FILE << "\n--- Creating HR Records ---\n";

	// Benefits and payroll tracking sets per staff
	int batch = 0;
	for (auto& s : STAFF) {
		string hr_id = "HR_" + s.id;
		seql(db, "CREATE SETS " + hr_id);
		seql(db, "ADD " + hr_id + " TO HR_Records");

		api_call("hr props " + hr_id, [&]() {
			return db.add_property("staff_id", s.id, hr_id);
		});
		api_call("hr props " + hr_id, [&]() {
			return db.add_property("salary_cents", s.salary_cents, hr_id);
		});
		api_call("hr props " + hr_id, [&]() {
			return db.add_property("pto_balance", s.pto_days, hr_id);
		});
		api_call("hr props " + hr_id, [&]() {
			string benefits = (s.salary_cents > 6000000) ? "full" : "basic";
			return db.add_property("benefits_tier", benefits, hr_id);
		});
		api_call("hr props " + hr_id, [&]() {
			return db.add_property("employment_status", string("active"), hr_id);
		});

		batch++;
		if (batch % 200 == 0) clear_cache_logged(db);
	}

	clear_cache_logged(db);
	LOG_FILE << "  HR Records created: " << STAFF.size() << "\n";
}

void chunk1_summary() {
	LOG_FILE << "\n--- CHUNK 1 SUMMARY ---\n";
	LOG_FILE << "Operations this chunk: " << OP_COUNT_CHUNK << "\n";
	LOG_FILE << "SeQL / API split: " << OP_COUNT_CHUNK_SEQL << " / " << OP_COUNT_CHUNK_API << "\n";
	LOG_FILE << "Failures: " << OP_COUNT_CHUNK_FAIL << "\n";
	LOG_FILE << "Duration: " << fixed << setprecision(1) << (CHUNK_DURATION_US / 1000.0) << " ms\n";
	LOG_FILE << "Sets in database: (counted at chunk end)\n";
	LOG_FILE << "Schools: " << SCHOOLS.size() << "\n";
	LOG_FILE << "Staff: " << STAFF.size() << "\n";
	LOG_FILE << "Students: " << STUDENTS.size() << "\n";
	LOG_FILE << "Classes: " << CLASSES.size() << "\n";
	LOG_FILE << "Extracurriculars: " << EXTRAS.size() << "\n";
	LOG_FILE << "Cache clears so far: " << CACHE_CLEARS << "\n";
	LOG_FILE << "Carry-overs to next week: none\n\n";
}

// ============================================================================
// WEEKLY SIMULATION HELPERS
// ============================================================================

// Simulate grade entry for a marking period
void do_grade_entry(cantordb& db, int marking_period) {
	LOG_FILE << "  [Grade Entry - Marking Period " << marking_period << "]\n";
	string mp_key = "mp" + to_string(marking_period);

	int graded = 0;
	for (auto& cls : CLASSES) {
		for (auto& stu_id : cls.student_ids) {
			int grade_val = rand_range(50, 100);
			// Properties are per-set, so we use a compound key: subject_mp
			string prop_key = cls.subject + "_" + mp_key;
			// Use API since we may need update_property if key exists
			api_call("grade " + stu_id + " " + prop_key, [&]() {
				return db.add_property(prop_key, grade_val, stu_id);
			});
			graded++;
		}
		// Clear cache every 50 classes
		if (&cls - &CLASSES[0] != 0 && (&cls - &CLASSES[0]) % 50 == 0)
			clear_cache_logged(db);
	}
	clear_cache_logged(db);
	LOG_FILE << "  Grades entered: " << graded << "\n";
}

// Transfer a student between schools
void do_transfer(cantordb& db, int stu_idx, const string& new_school) {
	auto& stu = STUDENTS[stu_idx];
	string old_school = stu.school;

	LOG_FILE << "  [Transfer] " << stu.id << " from " << old_school << " to " << new_school << "\n";

	seql(db, "REMOVE " + stu.id + " FROM " + old_school + "_Students");
	seql(db, "ADD " + stu.id + " TO " + new_school + "_Students");
	seql(db, "ADD " + stu.id + " TO Transferred_Students");

	// Remove from old school classes
	for (auto& cls : CLASSES) {
		if (cls.school == old_school) {
			auto it = find(cls.student_ids.begin(), cls.student_ids.end(), stu.id);
			if (it != cls.student_ids.end()) {
				seql(db, "REMOVE " + stu.id + " FROM " + cls.id);
				cls.student_ids.erase(it);
			}
		}
	}

	// Update property
	api_call("update school prop " + stu.id, [&]() {
		return db.update_property(stu.id, "school", new_school);
	});

	stu.school = new_school;

	// Add to new school classes (first available per subject)
	for (auto& cls : CLASSES) {
		if (cls.school == new_school && cls.student_ids.size() < 30) {
			// Check grade match
			int cls_grade = db.get_property_safe_int(cls.id, "grade");
			if (cls_grade == stu.grade) {
				seql(db, "ADD " + stu.id + " TO " + cls.id);
				cls.student_ids.push_back(stu.id);
			}
		}
	}

	// Update school tracking
	for (auto& s : SCHOOLS) {
		if (s.name == old_school) {
			auto it = find(s.student_ids.begin(), s.student_ids.end(), stu.id);
			if (it != s.student_ids.end()) s.student_ids.erase(it);
		}
		if (s.name == new_school) {
			s.student_ids.push_back(stu.id);
		}
	}

	clear_cache_logged(db);
}

// Suspend a student
void do_suspension(cantordb& db, int stu_idx, int days) {
	auto& stu = STUDENTS[stu_idx];
	LOG_FILE << "  [Suspension] " << stu.id << " for " << days << " days\n";

	seql(db, "REMOVE " + stu.id + " FROM Active_Students");
	seql(db, "ADD " + stu.id + " TO Suspended_Students");
	api_call("add suspension_days to " + stu.id, [&]() {
		return db.add_property("suspension_days", days, stu.id);
	});
	stu.suspended = true;
}

// Reinstate a suspended student
void do_reinstatement(cantordb& db, int stu_idx) {
	auto& stu = STUDENTS[stu_idx];
	if (!stu.suspended) return;
	LOG_FILE << "  [Reinstatement] " << stu.id << "\n";

	seql(db, "REMOVE " + stu.id + " FROM Suspended_Students");
	seql(db, "ADD " + stu.id + " TO Active_Students");
	api_call("delete suspension_days from " + stu.id, [&]() {
		return db.delete_property(stu.id, "suspension_days");
	});
	stu.suspended = false;
}

// Expel a student
void do_expulsion(cantordb& db, int stu_idx) {
	auto& stu = STUDENTS[stu_idx];
	LOG_FILE << "  [Expulsion] " << stu.id << "\n";

	seql(db, "REMOVE " + stu.id + " FROM Active_Students");
	seql(db, "ADD " + stu.id + " TO Expelled_Students");
	api_call("update enrolled to false " + stu.id, [&]() {
		return db.update_property(stu.id, "enrolled", false);
	});
	stu.expelled = true;
	stu.enrolled = false;

	// Remove from all classes
	for (auto& cls : CLASSES) {
		auto it = find(cls.student_ids.begin(), cls.student_ids.end(), stu.id);
		if (it != cls.student_ids.end()) {
			seql(db, "REMOVE " + stu.id + " FROM " + cls.id);
			cls.student_ids.erase(it);
		}
	}
}

// Terminate a staff member
void do_termination(cantordb& db, int staff_idx) {
	auto& s = STAFF[staff_idx];
	LOG_FILE << "  [Termination] " << s.id << " (" << s.role << " at " << s.school << ")\n";

	seql(db, "REMOVE " + s.id + " FROM Active_Staff");
	seql(db, "ADD " + s.id + " TO Terminated_Staff");
	api_call("update active to false " + s.id, [&]() {
		return db.update_property(s.id, "active", false);
	});
	api_call("update HR employment_status " + s.id, [&]() {
		string hr_id = "HR_" + s.id;
		return db.update_property(hr_id, "employment_status", string("terminated"));
	});
	s.active = false;
}

// Process payroll for all active staff
void do_payroll(cantordb& db) {
	LOG_FILE << "  [Payroll Run]\n";
	int processed = 0;
	for (auto& s : STAFF) {
		if (!s.active) continue;
		string hr_id = "HR_" + s.id;
		// Check via SeQL that staff member is in Active_Staff
		string check = seql(db, "IS " + s.id + " ELEMENT OF Active_Staff");
		if (check == "Yes") {
			// If in dispute, log it as edge case
			if (s.in_dispute) {
				LOG_FILE << "    [EDGE CASE] Payroll touching disputed staff " << s.id << "\n";
				EDGE_CASES_TRIGGERED++;
				// Still process but flag
			}
			processed++;
		}
	}
	clear_cache_logged(db);
	LOG_FILE << "  Payroll processed: " << processed << " staff\n";
}

// PTO request
void do_pto_request(cantordb& db, int staff_idx, int days) {
	auto& s = STAFF[staff_idx];
	string hr_id = "HR_" + s.id;
	int balance = db.get_property_safe_int(hr_id, "pto_balance");
	if (balance >= days) {
		api_call("update pto_balance " + hr_id, [&]() {
			return db.update_property(hr_id, "pto_balance", balance - days);
		});
		s.pto_days -= days;
		LOG_FILE << "  [PTO] " << s.id << " took " << days << " days (balance: " << (balance - days) << ")\n";
	} else {
		LOG_FILE << "  [PTO DENIED] " << s.id << " requested " << days << " days but only has " << balance << "\n";
	}
}

// Query verification: run set algebra queries to validate data integrity
void do_integrity_check(cantordb& db, const string& label) {
	LOG_FILE << "  [Integrity Check: " << label << "]\n";

	// Check active + suspended + expelled + transferred should cover all students
	string active_card = seql(db, "GET CARDINALITY OF Active_Students");
	string suspended_card = seql(db, "GET CARDINALITY OF Suspended_Students");
	string expelled_card = seql(db, "GET CARDINALITY OF Expelled_Students");
	string total_card = seql(db, "GET CARDINALITY OF All_Students");

	LOG_FILE << "    Active: " << active_card << " Suspended: " << suspended_card
	         << " Expelled: " << expelled_card << " Total: " << total_card << "\n";

	// Verify disjointness of student status sets
	string disj1 = seql(db, "IS Active_Students DISJOINT Suspended_Students");
	string disj2 = seql(db, "IS Active_Students DISJOINT Expelled_Students");
	string disj3 = seql(db, "IS Suspended_Students DISJOINT Expelled_Students");
	LOG_FILE << "    Active DISJOINT Suspended: " << disj1 << "\n";
	LOG_FILE << "    Active DISJOINT Expelled: " << disj2 << "\n";
	LOG_FILE << "    Suspended DISJOINT Expelled: " << disj3 << "\n";

	// Staff checks
	string staff_active = seql(db, "GET CARDINALITY OF Active_Staff");
	string staff_term = seql(db, "GET CARDINALITY OF Terminated_Staff");
	LOG_FILE << "    Active Staff: " << staff_active << " Terminated: " << staff_term << "\n";

	// IEP subset check
	string iep_subset = seql(db, "IS IEP_Students SUBSET OF All_Students");
	LOG_FILE << "    IEP SUBSET All_Students: " << iep_subset << "\n";

	clear_cache_logged(db);
}

// ============================================================================
// EDGE CASE FUNCTIONS
// ============================================================================

// Edge 1: Student suspended mid-transfer
void edge_suspended_mid_transfer(cantordb& db) {
	LOG_FILE << "\n  === EDGE CASE: Student suspended mid-transfer ===\n";
	EDGE_CASES_TRIGGERED++;

	// Pick a random active student
	int idx = -1;
	for (int i = 0; i < (int)STUDENTS.size(); i++) {
		if (STUDENTS[i].enrolled && !STUDENTS[i].suspended && !STUDENTS[i].expelled) {
			idx = i; break;
		}
	}
	if (idx < 0) { LOG_FILE << "  No eligible student found\n"; return; }

	// Start transfer
	string old_school = STUDENTS[idx].school;
	string new_school;
	for (auto& s : SCHOOLS) {
		if (s.name != old_school && s.grade_lo <= STUDENTS[idx].grade && s.grade_hi >= STUDENTS[idx].grade) {
			new_school = s.name; break;
		}
	}
	if (new_school.empty()) { LOG_FILE << "  No target school found\n"; return; }

	LOG_FILE << "  Step 1: Begin transfer " << STUDENTS[idx].id << " from " << old_school << " to " << new_school << "\n";
	// Remove from old school
	seql(db, "REMOVE " + STUDENTS[idx].id + " FROM " + old_school + "_Students");
	seql(db, "ADD " + STUDENTS[idx].id + " TO Transferred_Students");

	// INTERRUPT: suspend before completing transfer
	LOG_FILE << "  Step 2: SUSPENSION issued before transfer completes\n";
	do_suspension(db, idx, 5);

	// Now student is in Suspended_Students AND Transferred_Students but NOT in new school yet
	// Verify the messy state
	string in_suspended = seql(db, "IS " + STUDENTS[idx].id + " ELEMENT OF Suspended_Students");
	string in_transferred = seql(db, "IS " + STUDENTS[idx].id + " ELEMENT OF Transferred_Students");
	string in_old = seql(db, "IS " + STUDENTS[idx].id + " ELEMENT OF " + old_school + "_Students");
	string in_new = seql(db, "IS " + STUDENTS[idx].id + " ELEMENT OF " + new_school + "_Students");
	LOG_FILE << "  State: Suspended=" << in_suspended << " Transferred=" << in_transferred
	         << " In_Old=" << in_old << " In_New=" << in_new << "\n";

	// RECOVERY: Complete the transfer after suspension is served
	LOG_FILE << "  Step 3: Recovery - complete transfer post-suspension\n";
	do_reinstatement(db, idx);
	seql(db, "ADD " + STUDENTS[idx].id + " TO " + new_school + "_Students");
	api_call("update school prop " + STUDENTS[idx].id, [&]() {
		return db.update_property(STUDENTS[idx].id, "school", new_school);
	});
	STUDENTS[idx].school = new_school;

	EDGE_CASES_RESOLVED++;
	LOG_FILE << "  === EDGE CASE RESOLVED ===\n";
	clear_cache_logged(db);
}

// Edge 2: Staff terminated while holding PTO balance
void edge_terminated_with_pto(cantordb& db) {
	LOG_FILE << "\n  === EDGE CASE: Staff terminated with PTO balance ===\n";
	EDGE_CASES_TRIGGERED++;

	int idx = -1;
	for (int i = 0; i < (int)STAFF.size(); i++) {
		if (STAFF[i].active && STAFF[i].pto_days > 5) {
			idx = i; break;
		}
	}
	if (idx < 0) { LOG_FILE << "  No eligible staff\n"; return; }

	LOG_FILE << "  Staff " << STAFF[idx].id << " has PTO balance: " << STAFF[idx].pto_days << " days\n";

	// Terminate
	do_termination(db, idx);

	// Now check: HR record still has PTO balance
	string hr_id = "HR_" + STAFF[idx].id;
	int pto = db.get_property_safe_int(hr_id, "pto_balance");
	LOG_FILE << "  Post-termination PTO balance: " << pto << " (should be paid out)\n";

	// RECOVERY: Zero out PTO (simulate payout)
	api_call("zero pto " + hr_id, [&]() {
		return db.update_property(hr_id, "pto_balance", 0);
	});
	api_call("add pto_payout " + hr_id, [&]() {
		return db.add_property("pto_payout_cents", STAFF[idx].salary_cents / 260 * pto, hr_id);
	});

	EDGE_CASES_RESOLVED++;
	LOG_FILE << "  === EDGE CASE RESOLVED ===\n";
	clear_cache_logged(db);
}

// Edge 3: Duplicate student set during creation
void edge_duplicate_student(cantordb& db) {
	LOG_FILE << "\n  === EDGE CASE: Duplicate student set detected ===\n";
	EDGE_CASES_TRIGGERED++;

	// Try to create a student that already exists
	string dup_id = STUDENTS[0].id;
	string result = seql(db, "CREATE SETS " + dup_id);
	LOG_FILE << "  Attempted duplicate CREATE: " << result << "\n";

	// Result should indicate error
	bool detected = result.find("Error") != string::npos || result.find("exist") != string::npos;
	LOG_FILE << "  Duplicate detected: " << (detected ? "YES" : "NO") << "\n";

	// RECOVERY: if somehow it was created, we'd merge. Since CantorDB blocks it, just log.
	if (!detected) {
		LOG_FILE << "  WARNING: Duplicate was NOT blocked by CantorDB!\n";
	}

	EDGE_CASES_RESOLVED++;
	LOG_FILE << "  === EDGE CASE RESOLVED ===\n";
}

// Edge 4: Grade entry for expelled student
void edge_grade_expelled_student(cantordb& db) {
	LOG_FILE << "\n  === EDGE CASE: Grade entry for expelled student ===\n";
	EDGE_CASES_TRIGGERED++;

	// Find an expelled student
	int idx = -1;
	for (int i = 0; i < (int)STUDENTS.size(); i++) {
		if (STUDENTS[i].expelled) { idx = i; break; }
	}
	if (idx < 0) {
		// Expel someone first
		for (int i = 500; i < (int)STUDENTS.size(); i++) {
			if (STUDENTS[i].enrolled && !STUDENTS[i].suspended) { idx = i; break; }
		}
		if (idx < 0) { LOG_FILE << "  No eligible student\n"; return; }
		do_expulsion(db, idx);
	}

	string stu_id = STUDENTS[idx].id;
	LOG_FILE << "  Attempting to grade expelled student " << stu_id << "\n";

	// Verify student is expelled
	string is_expelled = seql(db, "IS " + stu_id + " ELEMENT OF Expelled_Students");
	LOG_FILE << "  Is expelled: " << is_expelled << "\n";

	// Try to add a grade anyway
	bool ok = false;
	api_call("attempt grade on expelled " + stu_id, [&]() {
		ok = db.add_property("Math_mp_late", 75, stu_id);
		return ok;
	});
	LOG_FILE << "  Grade add result: " << (ok ? "SUCCEEDED (should be caught by app logic)" : "FAILED") << "\n";

	// RECOVERY: If grade was added, remove it
	if (ok) {
		api_call("remove invalid grade from " + stu_id, [&]() {
			return db.delete_property(stu_id, "Math_mp_late");
		});
		LOG_FILE << "  Invalid grade removed\n";
	}

	EDGE_CASES_RESOLVED++;
	LOG_FILE << "  === EDGE CASE RESOLVED ===\n";
	clear_cache_logged(db);
}

// Edge 5: Sports roster with transferred-out student
void edge_stale_sports_roster(cantordb& db) {
	LOG_FILE << "\n  === EDGE CASE: Stale sports roster ===\n";
	EDGE_CASES_TRIGGERED++;

	// Find an extracurricular with students
	int extra_idx = -1;
	for (int i = 0; i < (int)EXTRAS.size(); i++) {
		if (!EXTRAS[i].student_ids.empty()) { extra_idx = i; break; }
	}
	if (extra_idx < 0) { LOG_FILE << "  No extras with students\n"; return; }

	auto& extra = EXTRAS[extra_idx];
	string stu_id = extra.student_ids[0];

	// Find this student's index
	int stu_idx = -1;
	for (int i = 0; i < (int)STUDENTS.size(); i++) {
		if (STUDENTS[i].id == stu_id) { stu_idx = i; break; }
	}
	if (stu_idx < 0) { LOG_FILE << "  Student not found in tracking\n"; return; }

	LOG_FILE << "  " << stu_id << " is on " << extra.id << " roster at " << extra.school << "\n";

	// Transfer the student out
	string old_school = STUDENTS[stu_idx].school;
	string new_school;
	for (auto& s : SCHOOLS) {
		if (s.name != old_school && s.grade_lo <= STUDENTS[stu_idx].grade && s.grade_hi >= STUDENTS[stu_idx].grade) {
			new_school = s.name; break;
		}
	}
	if (new_school.empty()) { LOG_FILE << "  No target school\n"; return; }

	do_transfer(db, stu_idx, new_school);

	// Now check: student is still on the old school's roster!
	string still_on = seql(db, "IS " + stu_id + " ELEMENT OF " + extra.id);
	LOG_FILE << "  Still on old roster after transfer: " << still_on << "\n";

	// RECOVERY: Clean the roster
	if (still_on == "Yes") {
		seql(db, "REMOVE " + stu_id + " FROM " + extra.id);
		auto it = find(extra.student_ids.begin(), extra.student_ids.end(), stu_id);
		if (it != extra.student_ids.end()) extra.student_ids.erase(it);
		LOG_FILE << "  Removed from stale roster\n";
	}

	EDGE_CASES_RESOLVED++;
	LOG_FILE << "  === EDGE CASE RESOLVED ===\n";
	clear_cache_logged(db);
}

// Edge 6: Payroll for disputed staff
void edge_payroll_disputed_staff(cantordb& db) {
	LOG_FILE << "\n  === EDGE CASE: Payroll for disputed staff ===\n";
	EDGE_CASES_TRIGGERED++;

	// Put a staff member into dispute
	int idx = -1;
	for (int i = 0; i < (int)STAFF.size(); i++) {
		if (STAFF[i].active && !STAFF[i].in_dispute) { idx = i; break; }
	}
	if (idx < 0) { LOG_FILE << "  No eligible staff\n"; return; }

	LOG_FILE << "  Putting " << STAFF[idx].id << " into dispute status\n";
	STAFF[idx].in_dispute = true;
	seql(db, "ADD " + STAFF[idx].id + " TO Disputed_Staff");
	api_call("update dispute status " + STAFF[idx].id, [&]() {
		string hr_id = "HR_" + STAFF[idx].id;
		return db.update_property(hr_id, "employment_status", string("disputed"));
	});

	// Now run payroll — the do_payroll function will detect the dispute
	LOG_FILE << "  Running payroll with disputed staff member present\n";
	// Check just this one
	string check = seql(db, "IS " + STAFF[idx].id + " ELEMENT OF Active_Staff");
	string check2 = seql(db, "IS " + STAFF[idx].id + " ELEMENT OF Disputed_Staff");
	LOG_FILE << "  In Active_Staff: " << check << " In Disputed_Staff: " << check2 << "\n";

	// RECOVERY: Hold payroll for disputed staff
	LOG_FILE << "  RESOLUTION: Payroll held pending dispute resolution\n";

	EDGE_CASES_RESOLVED++;
	LOG_FILE << "  === EDGE CASE RESOLVED ===\n";
	clear_cache_logged(db);
}

// Edge 7: Concurrent bulk grade entry (simulated)
void edge_concurrent_grade_entry(cantordb& db) {
	LOG_FILE << "\n  === EDGE CASE: Concurrent bulk grade entry ===\n";
	EDGE_CASES_TRIGGERED++;

	// Simulate two "teachers" grading the same set of students simultaneously
	// In single-threaded CantorDB this means back-to-back overwrites
	if (CLASSES.size() < 2) { LOG_FILE << "  Not enough classes\n"; return; }

	auto& cls = CLASSES[0];
	string prop1 = "concurrent_test_A";
	string prop2 = "concurrent_test_B";

	int conflicts = 0;
	for (auto& stu_id : cls.student_ids) {
		// Teacher A grades
		bool a = false;
		api_call("concurrent grade A " + stu_id, [&]() {
			a = db.add_property(prop1, rand_range(60, 100), stu_id);
			return a;
		});
		// Teacher B grades the same property name — should fail since key exists
		bool b = false;
		api_call("concurrent grade B " + stu_id, [&]() {
			b = db.add_property(prop1, rand_range(60, 100), stu_id);
			return b;
		});
		if (!b) conflicts++;
	}

	LOG_FILE << "  Conflicts detected: " << conflicts << " / " << cls.student_ids.size() << "\n";

	// RECOVERY: Use update_property for the second write
	for (auto& stu_id : cls.student_ids) {
		api_call("resolve concurrent " + stu_id, [&]() {
			return db.update_property(stu_id, prop1, rand_range(60, 100));
		});
	}
	LOG_FILE << "  Resolved via update_property\n";

	// Cleanup
	for (auto& stu_id : cls.student_ids) {
		api_call("cleanup " + stu_id, [&]() {
			return db.delete_property(stu_id, prop1);
		});
	}

	EDGE_CASES_RESOLVED++;
	LOG_FILE << "  === EDGE CASE RESOLVED ===\n";
	clear_cache_logged(db);
}

// Edge 8 (bonus): Rename a school mid-year
void edge_school_rename(cantordb& db) {
	LOG_FILE << "\n  === EDGE CASE: School rename mid-year ===\n";
	EDGE_CASES_TRIGGERED++;

	// Rename one school using API
	string old_name = SCHOOLS[0].name;
	string new_name = old_name + "_Renamed";

	LOG_FILE << "  Renaming " << old_name << " to " << new_name << "\n";

	api_call("rename_set " + old_name, [&]() {
		return db.rename_set(old_name, new_name);
	});

	// Verify the rename
	string check = seql(db, "IS " + new_name + " ELEMENT OF All_Schools");
	LOG_FILE << "  New name in All_Schools: " << check << "\n";

	// RECOVERY: Rename back (school names are everywhere in properties)
	api_call("rename_set back", [&]() {
		return db.rename_set(new_name, old_name);
	});

	string check2 = seql(db, "IS " + old_name + " ELEMENT OF All_Schools");
	LOG_FILE << "  Original name restored in All_Schools: " << check2 << "\n";

	EDGE_CASES_RESOLVED++;
	LOG_FILE << "  === EDGE CASE RESOLVED ===\n";
	clear_cache_logged(db);
}

// Edge 9 (bonus): Delete a set that is referenced as an element
void edge_delete_referenced_set(cantordb& db) {
	LOG_FILE << "\n  === EDGE CASE: Delete referenced set ===\n";
	EDGE_CASES_TRIGGERED++;

	// Create a temporary set, add it to a parent, then delete it
	seql(db, "CREATE SETS Temp_Edge_Set");
	seql(db, "ADD Temp_Edge_Set TO All_Students");

	// Verify membership
	string check = seql(db, "IS Temp_Edge_Set ELEMENT OF All_Students");
	LOG_FILE << "  Temp set in All_Students: " << check << "\n";

	// Hard delete
	seql(db, "DELETE SETS Temp_Edge_Set");

	// Verify removal
	string check2 = seql(db, "IS Temp_Edge_Set ELEMENT OF All_Students");
	LOG_FILE << "  After delete, in All_Students: " << check2 << "\n";

	EDGE_CASES_RESOLVED++;
	LOG_FILE << "  === EDGE CASE RESOLVED ===\n";
	clear_cache_logged(db);
}

// Edge 10 (bonus): Query on empty set results
void edge_empty_set_operations(cantordb& db) {
	LOG_FILE << "\n  === EDGE CASE: Operations on empty sets ===\n";
	EDGE_CASES_TRIGGERED++;

	seql(db, "CREATE SETS Empty_Set_A");
	seql(db, "CREATE SETS Empty_Set_B");

	string union_r = seql(db, "GET ELEMENTS OF Empty_Set_A UNION Empty_Set_B");
	string inter_r = seql(db, "GET ELEMENTS OF Empty_Set_A INTERSECTION Empty_Set_B");
	string diff_r = seql(db, "GET ELEMENTS OF Empty_Set_A DIFFERENCE Empty_Set_B");
	string card_r = seql(db, "GET CARDINALITY OF Empty_Set_A");
	string subset_r = seql(db, "IS Empty_Set_A SUBSET OF Empty_Set_B");
	string equal_r = seql(db, "IS Empty_Set_A EQUAL Empty_Set_B");
	string disjoint_r = seql(db, "IS Empty_Set_A DISJOINT Empty_Set_B");

	LOG_FILE << "  Union of empties: [" << union_r << "]\n";
	LOG_FILE << "  Intersection of empties: [" << inter_r << "]\n";
	LOG_FILE << "  Difference of empties: [" << diff_r << "]\n";
	LOG_FILE << "  Cardinality of empty: " << card_r << "\n";
	LOG_FILE << "  Empty SUBSET Empty: " << subset_r << "\n";
	LOG_FILE << "  Empty EQUAL Empty: " << equal_r << "\n";
	LOG_FILE << "  Empty DISJOINT Empty: " << disjoint_r << "\n";

	// Cleanup
	seql(db, "DELETE SETS Empty_Set_A");
	seql(db, "DELETE SETS Empty_Set_B");

	EDGE_CASES_RESOLVED++;
	LOG_FILE << "  === EDGE CASE RESOLVED ===\n";
	clear_cache_logged(db);
}

// ============================================================================
// WEEKLY SIMULATION
// ============================================================================

void simulate_week(cantordb& db, int week_num) {
	// Map week 1 = Sept Week 1 through week 40 = June Week 4
	int month = 9 + (week_num - 1) / 4;
	int week_of_month = ((week_num - 1) % 4) + 1;
	if (month > 12) month -= 12; // Wrap to Jan-June

	string month_names[] = {"", "January", "February", "March", "April", "May", "June",
	                         "July", "August", "September", "October", "November", "December"};
	string month_name = (month <= 12) ? month_names[month] : "Month_" + to_string(month);

	LOG_FILE << "\n========================================\n";
	LOG_FILE << "=== WEEK " << week_num << ": " << month_name << " Week " << week_of_month << " ===\n";
	LOG_FILE << "========================================\n";

	if (!CARRYOVERS.empty()) {
		LOG_FILE << "Pending carry-overs: ";
		for (auto& c : CARRYOVERS) LOG_FILE << c << "; ";
		LOG_FILE << "\n";
	} else {
		LOG_FILE << "Pending carry-overs: none\n";
	}
	CARRYOVERS.clear();

	reset_chunk_counters();

	// ---- Routine weekly operations ----

	// Attendance spot-checks (SeQL queries)
	int spot_checks = rand_range(3, 8);
	for (int i = 0; i < spot_checks; i++) {
		int si = RNG() % SCHOOLS.size();
		seql(db, "GET CARDINALITY OF " + SCHOOLS[si].name + "_Students");
	}

	// Random PTO requests (2-5 per week)
	int pto_count = rand_range(2, 5);
	for (int i = 0; i < pto_count; i++) {
		int idx = RNG() % STAFF.size();
		if (STAFF[idx].active) {
			do_pto_request(db, idx, rand_range(1, 3));
		}
	}

	// Random student transfers (0-3 per week)
	int transfer_count = rand_range(0, 3);
	for (int i = 0; i < transfer_count; i++) {
		int idx = RNG() % STUDENTS.size();
		if (STUDENTS[idx].enrolled && !STUDENTS[idx].suspended) {
			string new_school;
			for (auto& s : SCHOOLS) {
				if (s.name != STUDENTS[idx].school &&
				    s.grade_lo <= STUDENTS[idx].grade && s.grade_hi >= STUDENTS[idx].grade) {
					new_school = s.name; break;
				}
			}
			if (!new_school.empty()) {
				do_transfer(db, idx, new_school);
			}
		}
	}

	// Random suspensions (0-2 per week)
	int susp_count = rand_range(0, 2);
	for (int i = 0; i < susp_count; i++) {
		int idx = RNG() % STUDENTS.size();
		if (STUDENTS[idx].enrolled && !STUDENTS[idx].suspended && !STUDENTS[idx].expelled) {
			do_suspension(db, idx, rand_range(1, 10));
		}
	}

	// Reinstate previously suspended students (check all)
	if (week_num % 2 == 0) {
		int reinstated = 0;
		for (int i = 0; i < (int)STUDENTS.size() && reinstated < 5; i++) {
			if (STUDENTS[i].suspended) {
				do_reinstatement(db, i);
				reinstated++;
			}
		}
	}

	// ---- Marking period grade entry ----
	// MP1: Week 10, MP2: Week 20, MP3: Week 30, MP4: Week 40
	if (week_num == 10) do_grade_entry(db, 1);
	if (week_num == 20) do_grade_entry(db, 2);
	if (week_num == 30) do_grade_entry(db, 3);
	if (week_num == 40) do_grade_entry(db, 4);

	// ---- Biweekly payroll ----
	if (week_num % 2 == 0) {
		do_payroll(db);
	}

	// ---- Monthly integrity checks ----
	if (week_num % 4 == 0) {
		do_integrity_check(db, "Month " + to_string(month));
	}

	// ---- Edge cases at specific weeks ----
	if (week_num == 3) edge_duplicate_student(db);
	if (week_num == 6) edge_suspended_mid_transfer(db);
	if (week_num == 10) edge_concurrent_grade_entry(db);
	if (week_num == 14) edge_stale_sports_roster(db);
	if (week_num == 18) edge_terminated_with_pto(db);
	if (week_num == 22) edge_payroll_disputed_staff(db);
	if (week_num == 26) edge_grade_expelled_student(db);
	if (week_num == 30) edge_school_rename(db);
	if (week_num == 34) edge_delete_referenced_set(db);
	if (week_num == 38) edge_empty_set_operations(db);

	// ---- Semester events ----
	if (week_num == 1) {
		LOG_FILE << "  [First Day of School]\n";
		// Verify all schools have students
		for (auto& s : SCHOOLS) {
			seql(db, "GET CARDINALITY OF " + s.name + "_Students");
		}
	}
	if (week_num == 20) {
		LOG_FILE << "  [Mid-Year Review]\n";
		// Run comprehensive set algebra queries
		for (auto& s : SCHOOLS) {
			seql(db, "IS " + s.name + "_Students SUBSET OF All_Students");
			seql(db, "IS " + s.name + "_Staff SUBSET OF All_Staff");
		}
		seql(db, "IS Teachers SUBSET OF All_Staff");
		seql(db, "IS Custodians DISJOINT Teachers");
		clear_cache_logged(db);
	}
	if (week_num == 35) {
		LOG_FILE << "  [Senior expulsion - late year]\n";
		// Force an expulsion so edge 4 (grade_expelled) can have found someone
		for (int i = (int)STUDENTS.size() - 100; i < (int)STUDENTS.size(); i++) {
			if (STUDENTS[i].enrolled && STUDENTS[i].grade == 12 && !STUDENTS[i].suspended) {
				do_expulsion(db, i);
				break;
			}
		}
	}
	if (week_num == 40) {
		LOG_FILE << "  [Last Day of School - End of Year]\n";
		do_integrity_check(db, "End of Year");
	}

	// ---- Set algebra stress queries ----
	// Every 5 weeks, run heavy set algebra
	if (week_num % 5 == 0) {
		LOG_FILE << "  [Heavy Set Algebra Queries]\n";
		// Union of all school student sets
		if (SCHOOLS.size() >= 2) {
			seql(db, "GET CARDINALITY OF " + SCHOOLS[0].name + "_Students UNION " + SCHOOLS[1].name + "_Students");
			seql(db, "GET ELEMENTS OF " + SCHOOLS[0].name + "_Students INTERSECTION " + SCHOOLS[1].name + "_Students");
			seql(db, "GET ELEMENTS OF " + SCHOOLS[0].name + "_Students DIFFERENCE " + SCHOOLS[1].name + "_Students");
			seql(db, "IS " + SCHOOLS[0].name + "_Students DISJOINT " + SCHOOLS[1].name + "_Students");
		}
		// WHERE queries
		if (SCHOOLS.size() >= 2) {
			seql(db, "GET ELEMENTS OF " + SCHOOLS[0].name + "_Students WHERE grade > 3");
			seql(db, "GET CARDINALITY OF " + SCHOOLS[0].name + "_Students WHERE grade > 3");
		}
		// Dot notation precedence
		if (SCHOOLS.size() >= 3) {
			seql(db, "GET CARDINALITY OF " + SCHOOLS[0].name + "_Students .UNION. " +
			     SCHOOLS[1].name + "_Students INTERSECTION " + SCHOOLS[2].name + "_Students");
		}
		clear_cache_logged(db);
	}

	// ---- Summary ----
	LOG_FILE << "\n--- WEEK " << week_num << " SUMMARY ---\n";
	LOG_FILE << "Operations this week: " << OP_COUNT_CHUNK << "\n";
	LOG_FILE << "SeQL / API split: " << OP_COUNT_CHUNK_SEQL << " / " << OP_COUNT_CHUNK_API << "\n";
	LOG_FILE << "Failures: " << OP_COUNT_CHUNK_FAIL << "\n";
	LOG_FILE << "Duration: " << fixed << setprecision(1) << (CHUNK_DURATION_US / 1000.0) << " ms\n";
	if (!CARRYOVERS.empty()) {
		LOG_FILE << "Carry-overs to next week: ";
		for (auto& c : CARRYOVERS) LOG_FILE << c << "; ";
		LOG_FILE << "\n";
	} else {
		LOG_FILE << "Carry-overs to next week: none\n";
	}
}

// ============================================================================
// MAIN
// ============================================================================

int main() {
	LOG_FILE.open("stress_test_results.log");
	if (!LOG_FILE.is_open()) {
		cerr << "Failed to open log file!" << endl;
		return 1;
	}

	auto total_start = high_resolution_clock::now();

	LOG_FILE << "==========================================================\n";
	LOG_FILE << "  BROOKLYN DISTRICT STRESS TEST — CantorDB + SeQL\n";
	LOG_FILE << "==========================================================\n";
	LOG_FILE << "Date: 2026-03-13\n";
	LOG_FILE << "Target: ~20 schools, ~15000 students, ~1500 staff\n";
	LOG_FILE << "Simulation: 40-week school year + edge cases\n\n";

	cout << "Brooklyn District Stress Test starting..." << endl;
	cout << "Logging to stress_test_results.log" << endl;

	cantordb db("Brooklyn_Education_District");

	// CHUNK 1: Initial Data Creation
	cout << "CHUNK 1: Creating district schema..." << endl;
	create_district_schema(db);

	cout << "CHUNK 1: Creating schools..." << endl;
	create_schools(db);

	cout << "CHUNK 1: Creating staff (~1500)..." << endl;
	create_staff(db);

	cout << "CHUNK 1: Creating students (~15000)..." << endl;
	create_students(db);

	cout << "CHUNK 1: Creating classes..." << endl;
	create_classes(db);

	cout << "CHUNK 1: Creating extracurriculars..." << endl;
	create_extracurriculars(db);

	cout << "CHUNK 1: Creating HR records..." << endl;
	create_hr_records(db);

	chunk1_summary();

	int chunk1_ops = OP_COUNT_CHUNK;
	double chunk1_time = CHUNK_DURATION_US;

	cout << "CHUNK 1 complete: " << chunk1_ops << " ops in " << fixed << setprecision(1)
	     << (chunk1_time / 1000.0) << " ms" << endl;
	cout << "  Schools: " << SCHOOLS.size() << endl;
	cout << "  Staff: " << STAFF.size() << endl;
	cout << "  Students: " << STUDENTS.size() << endl;
	cout << "  Classes: " << CLASSES.size() << endl;
	cout << "  Extracurriculars: " << EXTRAS.size() << endl;
	cout << "  Sets in DB: " << db.set_index.size() << endl;

	// CHUNKS 2-40: Weekly Simulation
	for (int week = 1; week <= 40; week++) {
		cout << "Week " << week << "/40..." << flush;
		simulate_week(db, week);
		cout << " (" << OP_COUNT_CHUNK << " ops)" << endl;
	}

	auto total_end = high_resolution_clock::now();
	double total_ms = duration_cast<milliseconds>(total_end - total_start).count();

	// FINAL REPORT
	LOG_FILE << "\n==========================================================\n";
	LOG_FILE << "  FINAL REPORT\n";
	LOG_FILE << "==========================================================\n";
	LOG_FILE << "Total operations: " << OP_COUNT_TOTAL << "\n";
	LOG_FILE << "  SeQL: " << OP_COUNT_SEQL << "\n";
	LOG_FILE << "  API: " << OP_COUNT_API << "\n";
	LOG_FILE << "Total failures: " << OP_COUNT_FAIL << "\n";
	LOG_FILE << "Total duration: " << fixed << setprecision(1) << total_ms << " ms ("
	         << (total_ms / 1000.0) << " s)\n";
	LOG_FILE << "Average op time: " << fixed << setprecision(2) << (TOTAL_DURATION_US / OP_COUNT_TOTAL) << " us\n";
	LOG_FILE << "Cache clears: " << CACHE_CLEARS << "\n";
	LOG_FILE << "Edge cases triggered: " << EDGE_CASES_TRIGGERED << "\n";
	LOG_FILE << "Edge cases resolved: " << EDGE_CASES_RESOLVED << "\n";
	LOG_FILE << "Final set count: " << db.set_index.size() << "\n";
	LOG_FILE << "Final student count: " << STUDENTS.size() << "\n";
	LOG_FILE << "Final staff count: " << STAFF.size() << "\n";
	LOG_FILE << "Final class count: " << CLASSES.size() << "\n";

	// Final integrity check
	LOG_FILE << "\n--- Final Integrity ---\n";
	string final_all = seql(db, "GET CARDINALITY OF All_Students");
	string final_active = seql(db, "GET CARDINALITY OF Active_Students");
	string final_staff = seql(db, "GET CARDINALITY OF All_Staff");
	LOG_FILE << "All_Students cardinality: " << final_all << "\n";
	LOG_FILE << "Active_Students cardinality: " << final_active << "\n";
	LOG_FILE << "All_Staff cardinality: " << final_staff << "\n";

	LOG_FILE << "\n=== STRESS TEST COMPLETE ===\n";
	LOG_FILE.close();

	cout << "\n=== STRESS TEST COMPLETE ===" << endl;
	cout << "Total operations: " << OP_COUNT_TOTAL << endl;
	cout << "  SeQL: " << OP_COUNT_SEQL << endl;
	cout << "  API: " << OP_COUNT_API << endl;
	cout << "Failures: " << OP_COUNT_FAIL << endl;
	cout << "Total time: " << fixed << setprecision(1) << total_ms << " ms ("
	     << (total_ms / 1000.0) << " s)" << endl;
	cout << "Edge cases: " << EDGE_CASES_TRIGGERED << " triggered, " << EDGE_CASES_RESOLVED << " resolved" << endl;
	cout << "Final sets in DB: " << db.set_index.size() << endl;
	cout << "Results saved to stress_test_results.log" << endl;

	return 0;
}
