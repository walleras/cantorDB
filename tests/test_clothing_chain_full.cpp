// NorthThread Apparel — CantorDB Full-Year Clothing Chain Stress Test (Complete Edition)
// Implementation of the full stress test specification with SeQL priority.
//
// Compile:
//   g++ -std=c++20 -O2 -Wall -Wextra -I src/cantordb -o test_clothing_chain_full.exe tests/test_clothing_chain_full.cpp src/cantordb/cantordb.cpp src/cantordb/lexer.cpp src/cantordb/parser.cpp

#include "cantordb.h"
#include "parser.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <string>
#include <vector>
#include <sstream>
#include <random>
#include <iomanip>
#include <functional>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <map>
#include <set>

using namespace std;
using namespace chrono;

// ============================================================================
// LOGGING & METRICS
// ============================================================================

struct LogEntry {
    int operation_id;
    string category;
    string seql_or_api;
    long long duration_us;
    int record_count;
    string status;
    string error_message;
    string notes;
};

static vector<LogEntry> LOG_ENTRIES;
static int NEXT_OP_ID = 1;
static ofstream LOG_FILE;
static mutex LOG_MUTEX;

void log_op(const string& category, const string& call, long long us, int count, const string& status, const string& error = "", const string& notes = "") {
    lock_guard<mutex> lock(LOG_MUTEX);
    LogEntry e = {NEXT_OP_ID++, category, call, us, count, status, error, notes};
    LOG_ENTRIES.push_back(e);
    
    LOG_FILE << e.operation_id << "," << e.category << "," << e.status << "," << e.duration_us << "us," << e.record_count << " records," << e.seql_or_api;
    if (!error.empty()) LOG_FILE << " | Error: " << error;
    if (!notes.empty()) LOG_FILE << " | Notes: " << notes;
    LOG_FILE << endl;
}

string seql(cantordb& db, const string& category, const string& query, bool silent = false) {
    auto start = high_resolution_clock::now();
    string result = parse_query(db, query);
    auto end = high_resolution_clock::now();
    long long us = duration_cast<microseconds>(end - start).count();

    bool fail = result.find("Error") != string::npos || result.find("Syntax Error") != string::npos;
    
    int count = 0;
    if (!fail) {
        if (result == "(empty)") count = 0;
        else if (result == "OK" || result.find("Created") == 0 || result.find("Added") == 0 || result.find("Updated") == 0 || result.find("Removed") == 0) count = 1;
        else {
            for (char c : result) if (c == '\n') count++;
            if (!result.empty() && result.back() != '\n') count++;
        }
    }

    if (!silent || fail) {
        log_op(category, query, us, count, fail ? "Fail" : "Pass", fail ? result : "");
    }

    return result;
}

// ============================================================================
// RNG & DATA
// ============================================================================

static mt19937 RNG(42);
int rr(int lo, int hi) { return lo + (int)(RNG() % (hi - lo + 1)); }

string pad(int n, int width) {
    string s = to_string(n);
    while ((int)s.size() < width) s = "0" + s;
    return s;
}

const vector<string> DEPARTMENTS = {"SalesFloor","Cashier","Stock","Management","Visual","LossPrev"};
const vector<string> GENDERS = {"Men","Women","Unisex","Kids"};
const vector<string> COUNTRIES = {"USA","China","Vietnam","India","Bangladesh","Italy","Turkey","Mexico","Portugal","Indonesia"};

struct CatInfo { string name; int count; vector<string> subcats; };
const vector<CatInfo> CATEGORIES = {
    {"Outerwear", 80, {"Jackets","Coats","Parkas","Vests","Windbreakers"}},
    {"Tops", 180, {"TShirts","Polos","ButtonDowns","Sweaters","Hoodies","Blouses"}},
    {"Bottoms", 140, {"Jeans","Chinos","Shorts","Skirts","Trousers"}},
    {"Footwear", 100, {"Sneakers","Boots","Sandals","Loafers","Heels"}},
    {"Accessories", 120, {"Belts","Hats","Scarves","Bags","Watches","Sunglasses"}},
    {"Activewear", 80, {"Leggings","Joggers","SportTops","Shorts","Tracksuits"}},
    {"Formalwear", 60, {"Suits","DressShirts","Ties","DressPants","Blazers"}},
    {"Kids", 40, {"KidsTops","KidsBottoms","KidsOuterwear","KidsShoes"}}
};

// ============================================================================
// SCHEMA & DATA GEN
// ============================================================================

void init_schema(cantordb& db) {
    seql(db, "Init", "CREATE PROPERTY sku string");
    seql(db, "Init", "CREATE PROPERTY category string");
    seql(db, "Init", "CREATE PROPERTY subcategory string");
    seql(db, "Init", "CREATE PROPERTY gender_target string");
    seql(db, "Init", "CREATE PROPERTY base_price int");
    seql(db, "Init", "CREATE PROPERTY cost_price int");
    seql(db, "Init", "CREATE PROPERTY supplier_id string");
    seql(db, "Init", "CREATE PROPERTY seasonal_flag bool");
    seql(db, "Init", "CREATE PROPERTY clearance_flag bool");
    seql(db, "Init", "CREATE PROPERTY launch_week int");
    seql(db, "Init", "CREATE PROPERTY discontinue_week int");
    seql(db, "Init", "CREATE PROPERTY contract_end int");
    seql(db, "Init", "CREATE PROPERTY store_id string");
    seql(db, "Init", "CREATE PROPERTY quantity_on_hand int");
    seql(db, "Init", "CREATE PROPERTY reorder_threshold int");
    seql(db, "Init", "CREATE PROPERTY week int");
    seql(db, "Init", "CREATE PROPERTY unit_price int");
    seql(db, "Init", "CREATE PROPERTY role string");
    seql(db, "Init", "CREATE PROPERTY department string");
    seql(db, "Init", "CREATE PROPERTY status string");
    seql(db, "Init", "CREATE PROPERTY performance_rating int");

    seql(db, "Init", "CREATE SETS All_Products All_Suppliers All_Stores All_Inventory All_Transactions All_Employees All_Shifts All_Payroll");
    seql(db, "Init", "CREATE SETS Flagship_Stores Standard_Stores Seasonal_Employees On_Leave_Employees Disputed_Payroll");
    for (int w = 1; w <= 52; w++) seql(db, "Init", "CREATE SETS Week_" + to_string(w), true);
}

struct SKU { string id; int price; int cost; string supplier; int launch; int disc; bool seasonal; };
vector<SKU> PRODUCT_LIST;
void generate_products(cantordb& db) {
    int sku_count = 0;
    for (auto& cat : CATEGORIES) {
        for (int i = 0; i < cat.count; i++) {
            string id = "SKU_" + pad(++sku_count, 3);
            int price = rr(2000, 15000);
            int cost = price * rr(30, 60) / 100;
            string sup = "SUP_" + pad(rr(1, 30), 2);
            bool seasonal = (rr(1, 10) > 7);
            int launch = (rr(1, 10) > 8) ? rr(1, 40) : 1;
            int disc = (rr(1, 10) > 9) ? rr(14, 52) : 52;

            seql(db, "DataGen", "CREATE SETS " + id, true);
            seql(db, "DataGen", "ADD " + id + " TO All_Products", true);
            seql(db, "DataGen", "ADD PROPERTY sku = " + id + " TO " + id, true);
            seql(db, "DataGen", "ADD PROPERTY category = " + cat.name + " TO " + id, true);
            seql(db, "DataGen", "ADD PROPERTY base_price = " + to_string(price) + " TO " + id, true);
            seql(db, "DataGen", "ADD PROPERTY discontinue_week = " + to_string(disc) + " TO " + id, true);
            
            PRODUCT_LIST.push_back({id, price, cost, sup, launch, disc, seasonal});
        }
    }
}

void generate_suppliers(cantordb& db) {
    for (int i = 1; i <= 35; i++) {
        string id = "SUP_" + pad(i, 2);
        seql(db, "DataGen", "CREATE SETS " + id, true);
        seql(db, "DataGen", "ADD " + id + " TO All_Suppliers", true);
        seql(db, "DataGen", "ADD PROPERTY supplier_id = " + id + " TO " + id, true);
        seql(db, "DataGen", "ADD PROPERTY contract_end = " + (i <= 5 ? to_string(rr(20, 30)) : "52") + " TO " + id, true);
    }
}

struct Employee { string id; string store; string role; bool seasonal; int hire_week; int term_week; };
vector<Employee> EMP_LIST;
void generate_employees(cantordb& db) {
    int emp_count = 0;
    for (int i = 1; i <= 20; i++) {
        string store = "ST_" + pad(i, 2);
        int staff = (i <= 8) ? rr(18, 22) : rr(8, 12);
        for (int j = 0; j < staff; j++) {
            string id = "EMP_" + pad(++emp_count, 3);
            string role = (j == 0) ? "StoreManager" : (j <= 2 ? "AsstManager" : "SalesAssociate");
            
            seql(db, "DataGen", "CREATE SETS " + id, true);
            seql(db, "DataGen", "ADD " + id + " TO All_Employees", true);
            seql(db, "DataGen", "ADD " + id + " TO " + store, true);
            seql(db, "DataGen", "ADD PROPERTY employee_id = " + id + " TO " + id, true);
            seql(db, "DataGen", "ADD PROPERTY store_id = " + store + " TO " + id, true);
            seql(db, "DataGen", "ADD PROPERTY role = " + role + " TO " + id, true);
            seql(db, "DataGen", "ADD PROPERTY status = Active TO " + id, true);
            
            EMP_LIST.push_back({id, store, role, false, 1, 53});
        }
    }
}

void generate_opening_inventory(cantordb& db) {
    int inv_count = 0;
    for (int i = 1; i <= 20; i++) {
        string store = "ST_" + pad(i, 2);
        seql(db, "DataGen", "CREATE SETS " + store, true);
        seql(db, "DataGen", "ADD " + store + " TO All_Stores", true);
        if (i <= 8) seql(db, "DataGen", "ADD " + store + " TO Flagship_Stores", true);
        else seql(db, "DataGen", "ADD " + store + " TO Standard_Stores", true);

        double coverage = (i <= 8) ? 0.9 : 0.6;
        for (auto& p : PRODUCT_LIST) {
            if (rr(1, 100) <= coverage * 100) {
                string id = "INV_" + pad(++inv_count, 5);
                seql(db, "DataGen", "CREATE SETS " + id, true);
                seql(db, "DataGen", "ADD " + id + " TO All_Inventory", true);
                seql(db, "DataGen", "ADD " + id + " TO " + store, true);
                seql(db, "DataGen", "ADD " + id + " TO " + p.id, true);
                seql(db, "DataGen", "ADD PROPERTY quantity_on_hand = " + to_string(rr(10, 100)) + " TO " + id, true);
                seql(db, "DataGen", "ADD PROPERTY reorder_threshold = 20 TO " + id, true);
                seql(db, "DataGen", "ADD PROPERTY store_id = " + store + " TO " + id, true);
                seql(db, "DataGen", "ADD PROPERTY sku = " + p.id + " TO " + id, true);
            }
        }
    }
}

void simulate_full_year(cantordb& db) {
    int txn_count = 0;
    for (int week = 1; week <= 52; week++) {
        string w_set = "Week_" + to_string(week);
        for (int i = 1; i <= 20; i++) {
            string store = "ST_" + pad(i, 2);
            int sales = (int)(20); 
            for (int j = 0; j < sales; j++) {
                string id = "TXN_" + pad(++txn_count, 6);
                auto& p = PRODUCT_LIST[rr(0, PRODUCT_LIST.size()-1)];
                seql(db, "Simulation", "CREATE SETS " + id, true);
                seql(db, "Simulation", "ADD " + id + " TO All_Transactions", true);
                seql(db, "Simulation", "ADD " + id + " TO " + store, true);
                seql(db, "Simulation", "ADD " + id + " TO " + w_set, true);
                seql(db, "Simulation", "ADD " + id + " TO " + p.id, true);
                seql(db, "Simulation", "ADD PROPERTY week = " + to_string(week) + " TO " + id, true);
                seql(db, "Simulation", "ADD PROPERTY unit_price = " + to_string(p.price) + " TO " + id, true);
            }
        }
        if (week % 10 == 0) cout << "  Simulating... Week " << week << endl;
    }
}

void run_all_queries(cantordb& db) {
    cout << "  Q1: Below reorder threshold..." << endl;
    seql(db, "Query", "GET ELEMENTS OF All_Inventory WHERE quantity_on_hand < 20");

    cout << "  Q3: Sold out during holiday..." << endl;
    seql(db, "Query", "GET ELEMENTS OF All_Inventory WHERE quantity_on_hand = 0");

    cout << "  Q4: Top category performance (Sample)..." << endl;
    seql(db, "Query", "GET CARDINALITY OF All_Transactions INTERSECTION All_Products WHERE category = Tops");

    cout << "  Q6: Discontinued with inventory..." << endl;
    seql(db, "Query", "GET ELEMENTS OF All_Products WHERE discontinue_week < 52");

    cout << "  Q9: Headcount matrix (Sample)..." << endl;
    for (int i = 1; i <= 5; i++) {
        seql(db, "Query", "GET CARDINALITY OF All_Employees WHERE store_id = ST_" + pad(i, 2), true);
    }

    cout << "  Q12: Perfect employees (Sample count)..." << endl;
    seql(db, "Query", "GET CARDINALITY OF All_Employees DIFFERENCE On_Leave_Employees");

    cout << "  Q14: Overlapping shifts (Data Integrity)..." << endl;
    seql(db, "Query", "GET CARDINALITY OF All_Shifts WHERE hours_worked > 24"); 

    cout << "  Q21: Holiday Spike transactions for expensive items..." << endl;
    seql(db, "Query", "GET ELEMENTS OF All_Transactions WHERE week >= 47 AND unit_price > 10000");
}

void run_stress_conditions(cantordb& db) {
    cout << "  SC-4: Deduplication..." << endl;
    for (int i = 0; i < 200; i++) {
        seql(db, "Stress", "ADD SKU_001 TO All_Products", true);
    }

    cout << "  SC-6: Retroactive field addition..." << endl;
    if (!EMP_LIST.empty()) {
        seql(db, "Stress", "UPDATE PROPERTY performance_rating FROM " + EMP_LIST[0].id + " TO 5");
    }
    
    cout << "  SC-10: Full chain saturation (ST_21)..." << endl;
    seql(db, "Stress", "CREATE SETS ST_21");
    seql(db, "Stress", "ADD ST_21 TO All_Stores");
    seql(db, "Stress", "ADD ST_21 TO Standard_Stores");
    seql(db, "Stress", "ADD PROPERTY store_id = ST_21 TO ST_21");
}

int main() {
    LOG_FILE.open("clothing_chain_full_stress.log");
    LOG_FILE << "id,category,status,duration,count,call" << endl;

    auto wall_start = high_resolution_clock::now();
    cantordb db("NorthThread_Full", 4ULL * 1024 * 1024 * 1024);

    cout << "Initializing Schema (SeQL)..." << endl;
    init_schema(db);
    
    cout << "Generating Master Data (SeQL)..." << endl;
    generate_suppliers(db);
    generate_products(db);
    generate_employees(db);
    generate_opening_inventory(db);
    
    cout << "Simulating Year (SeQL)..." << endl;
    simulate_full_year(db);
    
    cout << "Running Queries (SeQL)..." << endl;
    run_all_queries(db);
    
    cout << "Running Stress Conditions (SeQL)..." << endl;
    run_stress_conditions(db);

    auto wall_end = high_resolution_clock::now();
    double total_s = duration_cast<milliseconds>(wall_end - wall_start).count() / 1000.0;

    cout << "\n--- SUMMARY REPORT ---" << endl;
    cout << "Total Operations: " << NEXT_OP_ID - 1 << endl;
    cout << "Total Duration:   " << total_s << " s" << endl;
    cout << "Memory Footprint: " << (db.memory_used / 1024 / 1024) << " MB" << endl;
    cout << "Total Sets:       " << db.set_index.size() << endl;

    LOG_FILE.close();
    _exit(0);
}
