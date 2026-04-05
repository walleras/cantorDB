// CantorDB Destructor Stress Test
// Goal: Create 2,000,000 sets and let the destructor clean them up naturally.
//
// Compile:
//   g++ -std=c++20 -O2 -Wall -Wextra -I src/cantordb -o test_destructor_stress.exe tests/test_destructor_stress.cpp src/cantordb/cantordb.cpp src/cantordb/lexer.cpp src/cantordb/parser.cpp

#include "cantordb.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

using namespace std;
using namespace chrono;

int main() {
    // 4GB limit
    cantordb db("DestructorStress", 4ULL * 1024 * 1024 * 1024);
    
    int TARGET_SETS = 2000000;
    
    cout << "Starting Destructor Stress Test..." << endl;
    cout << "Target: " << TARGET_SETS << " sets." << endl;

    auto start = high_resolution_clock::now();

    // Create a "Master" set to force some membership depth
    db.create_set("MasterSet");

    for (int i = 0; i < TARGET_SETS; i++) {
        string name = "Set_" + to_string(i);
        if (!db.create_set(name)) {
            cerr << "Failed to create set " << i << ": " << db.error_message << endl;
            break;
        }
        
        // Link to MasterSet to populate member_of/has_element vectors
        // This ensures the destructor actually has work to do (clearing vectors)
        if (i % 100 == 0) {
             db.add_member("MasterSet", name);
        }

        if (i % 200000 == 0 && i > 0) {
            cout << "  Created " << i << " sets... (Mem: " << (db.memory_used / 1024 / 1024) << " MB)" << endl;
        }
    }

    auto end = high_resolution_clock::now();
    double dur = duration_cast<milliseconds>(end - start).count() / 1000.0;

    cout << "Finished creating " << db.set_index.size() << " sets in " << dur << "s." << endl;
    cout << "Current Memory Usage (Tracked): " << (db.memory_used / 1024 / 1024) << " MB" << endl;
    cout << "Exiting main() now... forcing destructor..." << endl;
    
    // NO _exit(0) here! We WANT the destructor to run.
    return 0;
}
