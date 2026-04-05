// =============================================================================
// HZRI Stress Test — Hartwell Zoological Research Institute (Research Edition)
// Stress test for CantorDB v0.1.0
// =============================================================================
// Does NOT modify CantorDB source. Uses C++ API for bulk construction,
// SeQL (parse_query) for all queries and stress conditions.
// Logs every operation to CSV + prints summary to stdout.
// =============================================================================

#include "cantordb.h"
#include "parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <chrono>
#include <random>
#include <map>
#include <set>
#include <algorithm>
#include <iomanip>
#include <numeric>
#include <cmath>
#include <functional>

using namespace std;
using namespace chrono;

// =============================================================================
// LOGGING
// =============================================================================

struct OpLog {
    string op_id, category, seql_or_api;
    long long duration_us;
    int result_count;
    string status, notes;
};

static vector<OpLog> g_log;
static int g_op_counter = 0;
static int g_pass = 0, g_fail = 0, g_correctness_fail = 0;
static int g_api_fallback = 0, g_partial_hybrid = 0;

string next_id(const string& cat) {
    return cat + "-" + to_string(++g_op_counter);
}

void log_op(const string& cat, const string& desc, long long us, int count,
            const string& status, const string& notes = "") {
    g_log.push_back({next_id(cat), cat, desc, us, count, status, notes});
    if (status == "Pass") g_pass++;
    else if (status == "Fail") g_fail++;
    else if (status == "Correctness-Failure") g_correctness_fail++;
    else if (status == "API-Fallback") g_api_fallback++;
    else if (status == "Partial-Hybrid") g_partial_hybrid++;
}

int count_lines(const string& s) {
    if (s.empty()) return 0;
    int n = 0;
    for (char c : s) { if (c == '\n') n++; }
    if (!s.empty() && s.back() != '\n') n++;
    return n;
}

// Execute SeQL query with logging
string sq(cantordb& db, const string& query, const string& cat,
          const string& notes = "", const string& force_status = "") {
    auto t0 = high_resolution_clock::now();
    string r = parse_query(db, query);
    auto t1 = high_resolution_clock::now();
    long long us = duration_cast<microseconds>(t1 - t0).count();
    bool err = (r.find("Error") != string::npos) || (r.find("error") != string::npos);
    string status = force_status.empty() ? (err ? "Fail" : "Pass") : force_status;
    int cnt = err ? 0 : count_lines(r);
    log_op(cat, query, us, cnt, status, notes);
    return r;
}

// Parse elements string into vector
vector<string> to_vec(const string& s) {
    vector<string> v;
    istringstream iss(s);
    string line;
    while (getline(iss, line)) {
        if (!line.empty()) v.push_back(line);
    }
    return v;
}

// =============================================================================
// SPECIES DATA (80 species)
// =============================================================================

struct SpeciesInfo {
    const char *code, *name, *common, *binomial, *cls, *iucn, *diet, *biome, *region;
    bool venomous, nocturnal, solitary;
    int animal_count;
};

static const SpeciesInfo SPECIES[] = {
    // --- Mammalia (20 species, 75 animals) ---
    {"TGR","Tiger","Tiger","Panthera tigris","Mammalia","EN","Carnivore","Tropical","Asia",0,1,1,5},
    {"LIO","Lion","Lion","Panthera leo","Mammalia","VU","Carnivore","Savanna","Africa",0,0,0,5},
    {"WLF","Wolf","Gray Wolf","Canis lupus","Mammalia","LC","Carnivore","Temperate","Americas",0,0,0,4},
    {"EAF","ElephantAf","African Elephant","Loxodonta africana","Mammalia","EN","Herbivore","Savanna","Africa",0,0,0,5},
    {"EAS","ElephantAs","Asian Elephant","Elephas maximus","Mammalia","EN","Herbivore","Tropical","Asia",0,0,0,4},
    {"GIR","Giraffe","Giraffe","Giraffa camelopardalis","Mammalia","VU","Herbivore","Savanna","Africa",0,0,0,4},
    {"ZBR","Zebra","Plains Zebra","Equus quagga","Mammalia","NT","Herbivore","Savanna","Africa",0,0,0,4},
    {"GOR","Gorilla","Western Gorilla","Gorilla gorilla","Mammalia","CR","Herbivore","Tropical","Africa",0,0,0,3},
    {"CHP","Chimpanzee","Chimpanzee","Pan troglodytes","Mammalia","EN","Omnivore","Tropical","Africa",0,0,0,4},
    {"ORA","Orangutan","Bornean Orangutan","Pongo pygmaeus","Mammalia","CR","Omnivore","Tropical","Asia",0,0,1,3},
    {"PBR","PolarBear","Polar Bear","Ursus maritimus","Mammalia","VU","Carnivore","Arctic","Americas",0,0,1,3},
    {"GBR","GrizzlyBear","Grizzly Bear","Ursus arctos","Mammalia","LC","Omnivore","Temperate","Americas",0,0,1,3},
    {"RPD","RedPanda","Red Panda","Ailurus fulgens","Mammalia","EN","Herbivore","Temperate","Asia",0,0,1,3},
    {"SLP","SnowLeopard","Snow Leopard","Panthera uncia","Mammalia","VU","Carnivore","Alpine","Asia",0,1,1,3},
    {"JAG","Jaguar","Jaguar","Panthera onca","Mammalia","NT","Carnivore","Tropical","Americas",0,1,1,3},
    {"HIP","Hippo","Hippopotamus","Hippopotamus amphibius","Mammalia","VU","Herbivore","Wetland","Africa",0,0,0,4},
    {"RHI","Rhino","White Rhinoceros","Ceratotherium simum","Mammalia","NT","Herbivore","Savanna","Africa",0,0,1,3},
    {"KAN","Kangaroo","Red Kangaroo","Macropus rufus","Mammalia","LC","Herbivore","Arid","Asia",0,0,0,4},
    {"FBT","FruitBat","Large Flying Fox","Pteropus vampyrus","Mammalia","NT","Herbivore","Tropical","Asia",0,1,0,3},
    {"PAN","Pangolin","Sunda Pangolin","Manis javanica","Mammalia","CR","Omnivore","Tropical","Asia",0,1,1,3},
    // --- Aves (15 species, 55 animals) ---
    {"BEG","BaldEagle","Bald Eagle","Haliaeetus leucocephalus","Aves","LC","Carnivore","Temperate","Americas",0,0,1,4},
    {"EPN","EmperorPenguin","Emperor Penguin","Aptenodytes forsteri","Aves","NT","Carnivore","Antarctic","Americas",0,0,0,4},
    {"FLM","Flamingo","Greater Flamingo","Phoenicopterus roseus","Aves","LC","Omnivore","Wetland","Africa",0,0,0,5},
    {"SMC","ScarletMacaw","Scarlet Macaw","Ara macao","Aves","LC","Herbivore","Tropical","Americas",0,0,0,4},
    {"GHO","GreatHornedOwl","Great Horned Owl","Bubo virginianus","Aves","LC","Carnivore","Temperate","Americas",0,1,1,3},
    {"TOU","Toucan","Toco Toucan","Ramphastos toco","Aves","LC","Omnivore","Tropical","Americas",0,0,0,4},
    {"PEL","Pelican","Brown Pelican","Pelecanus occidentalis","Aves","LC","Carnivore","Coastal","Americas",0,0,0,3},
    {"CAS","Cassowary","Southern Cassowary","Casuarius casuarius","Aves","VU","Omnivore","Tropical","Asia",0,0,1,3},
    {"CON","Condor","Andean Condor","Vultur gryphus","Aves","NT","Carnivore","Alpine","Americas",0,0,0,4},
    {"WCR","WhoopingCrane","Whooping Crane","Grus americana","Aves","EN","Omnivore","Wetland","Americas",0,0,0,3},
    {"AGP","AfGreyParrot","African Grey Parrot","Psittacus erithacus","Aves","EN","Herbivore","Tropical","Africa",0,0,0,4},
    {"HRN","Hornbill","Rhinoceros Hornbill","Buceros rhinoceros","Aves","VU","Omnivore","Tropical","Asia",0,0,0,3},
    {"SBR","SecretaryBird","Secretary Bird","Sagittarius serpentarius","Aves","EN","Carnivore","Savanna","Africa",0,0,0,3},
    {"KWI","Kiwi","North Island Kiwi","Apteryx mantelli","Aves","VU","Omnivore","Temperate","Asia",0,1,1,3},
    {"OST","Ostrich","Common Ostrich","Struthio camelus","Aves","LC","Omnivore","Savanna","Africa",0,0,0,4},
    // --- Reptilia (12 species, 45 animals) ---
    {"KMD","KomodoDragon","Komodo Dragon","Varanus komodoensis","Reptilia","EN","Carnivore","Tropical","Asia",0,0,1,4},
    {"GST","GreenSeaTurtle","Green Sea Turtle","Chelonia mydas","Reptilia","EN","Herbivore","Marine","Americas",0,0,1,4},
    {"KCB","KingCobra","King Cobra","Ophiophagus hannah","Reptilia","VU","Carnivore","Tropical","Asia",1,0,1,4},
    {"SCR","SaltwaterCroc","Saltwater Crocodile","Crocodylus porosus","Reptilia","LC","Carnivore","Wetland","Asia",0,0,1,4},
    {"GAT","GalapagosTort","Galapagos Tortoise","Chelonoidis nigra","Reptilia","VU","Herbivore","Volcanic","Americas",0,0,1,4},
    {"CML","Chameleon","Veiled Chameleon","Chamaeleo calyptratus","Reptilia","LC","Carnivore","Tropical","Africa",0,0,1,3},
    {"IGU","Iguana","Green Iguana","Iguana iguana","Reptilia","LC","Herbivore","Tropical","Americas",0,0,0,4},
    {"ANA","Anaconda","Green Anaconda","Eunectes murinus","Reptilia","LC","Carnivore","Tropical","Americas",0,1,1,3},
    {"RTL","Rattlesnake","Western Diamondback","Crotalus atrox","Reptilia","LC","Carnivore","Arid","Americas",1,1,1,4},
    {"GIL","GilaMonster","Gila Monster","Heloderma suspectum","Reptilia","NT","Carnivore","Arid","Americas",1,1,1,4},
    {"GHA","Gharial","Gharial","Gavialis gangeticus","Reptilia","CR","Carnivore","Wetland","Asia",0,0,1,4},
    {"TUA","Tuatara","Tuatara","Sphenodon punctatus","Reptilia","VU","Carnivore","Temperate","Asia",0,1,1,3},
    // --- Amphibia (8 species, 30 animals) ---
    {"PDF","PoisonDartFrog","Poison Dart Frog","Dendrobates tinctorius","Amphibia","LC","Carnivore","Tropical","Americas",1,0,0,4},
    {"AXL","Axolotl","Axolotl","Ambystoma mexicanum","Amphibia","CR","Carnivore","Freshwater","Americas",0,0,0,4},
    {"GSA","GiantSalamander","Japanese Giant Salamander","Andrias japonicus","Amphibia","VU","Carnivore","Freshwater","Asia",0,1,0,4},
    {"RTF","RedEyedTreeFrog","Red-Eyed Tree Frog","Agalychnis callidryas","Amphibia","LC","Carnivore","Tropical","Americas",0,1,0,4},
    {"CTD","CaneToad","Cane Toad","Rhinella marina","Amphibia","LC","Omnivore","Tropical","Americas",1,1,0,4},
    {"FSA","FireSalamander","Fire Salamander","Salamandra salamandra","Amphibia","LC","Carnivore","Temperate","Africa",0,1,0,4},
    {"HLB","Hellbender","Hellbender","Cryptobranchus alleganiensis","Amphibia","NT","Carnivore","Freshwater","Americas",0,1,0,3},
    {"CAE","Caecilian","Caecilian","Typhlonectes natans","Amphibia","LC","Carnivore","Freshwater","Americas",0,1,1,3},
    // --- Actinopterygii (10 species, 35 animals) ---
    {"CLN","Clownfish","Clownfish","Amphiprion ocellaris","Actinopterygii","LC","Omnivore","Marine","Asia",0,0,0,4},
    {"PIR","Piranha","Red-bellied Piranha","Pygocentrus nattereri","Actinopterygii","LC","Carnivore","Freshwater","Americas",0,0,0,4},
    {"SHR","Seahorse","Spotted Seahorse","Hippocampus kuda","Actinopterygii","VU","Carnivore","Marine","Asia",0,0,0,3},
    {"ARW","Arowana","Silver Arowana","Osteoglossum bicirrhosum","Actinopterygii","LC","Carnivore","Freshwater","Americas",0,0,0,4},
    {"PUF","Pufferfish","Fahaka Pufferfish","Tetraodon lineatus","Actinopterygii","LC","Omnivore","Freshwater","Africa",1,0,0,3},
    {"EEL","ElectricEel","Electric Eel","Electrophorus electricus","Actinopterygii","LC","Carnivore","Freshwater","Americas",0,1,0,4},
    {"LNG","Lungfish","West African Lungfish","Protopterus annectens","Actinopterygii","LC","Omnivore","Freshwater","Africa",0,0,0,3},
    {"STU","Sturgeon","Atlantic Sturgeon","Acipenser sturio","Actinopterygii","CR","Omnivore","Freshwater","Americas",0,0,0,4},
    {"DSC","Discus","Discus","Symphysodon discus","Actinopterygii","LC","Omnivore","Freshwater","Americas",0,0,0,3},
    {"ARP","Arapaima","Arapaima","Arapaima gigas","Actinopterygii","LC","Carnivore","Freshwater","Americas",0,0,0,3},
    // --- Insecta (10 species, 28 animals) ---
    {"MBF","MonarchButterfly","Monarch Butterfly","Danaus plexippus","Insecta","EN","Herbivore","Temperate","Americas",0,0,0,3},
    {"ATM","AtlasMoth","Atlas Moth","Attacus atlas","Insecta","LC","Herbivore","Tropical","Asia",0,1,0,3},
    {"HRC","HerculesBeetle","Hercules Beetle","Dynastes hercules","Insecta","LC","Herbivore","Tropical","Americas",0,1,0,3},
    {"LCA","LeafCutterAnt","Leaf-Cutter Ant","Atta cephalotes","Insecta","LC","Herbivore","Tropical","Americas",0,0,0,3},
    {"PMT","PrayingMantis","Praying Mantis","Mantis religiosa","Insecta","LC","Carnivore","Temperate","Africa",0,0,1,3},
    {"WST","WalkingStick","Walking Stick","Phasmatodea sp","Insecta","LC","Herbivore","Tropical","Asia",0,1,0,3},
    {"JWB","JewelBeetle","Jewel Beetle","Chrysochroa sp","Insecta","LC","Herbivore","Tropical","Asia",0,0,0,2},
    {"GOL","GoliathBeetle","Goliath Beetle","Goliathus goliathus","Insecta","LC","Herbivore","Tropical","Africa",0,0,0,3},
    {"DRG","Dragonfly","Emperor Dragonfly","Anax imperator","Insecta","LC","Carnivore","Wetland","Africa",0,0,0,3},
    {"DNG","DungBeetle","Dung Beetle","Scarabaeus sacer","Insecta","LC","Omnivore","Savanna","Africa",0,0,0,2},
    // --- Arachnida (5 species, 12 animals) ---
    {"TAR","Tarantula","Goliath Birdeater","Theraphosa blondi","Arachnida","LC","Carnivore","Tropical","Americas",1,1,1,3},
    {"ESC","EmperorScorpion","Emperor Scorpion","Pandinus imperator","Arachnida","LC","Carnivore","Tropical","Africa",1,1,1,3},
    {"BWD","BlackWidow","Black Widow","Latrodectus mactans","Arachnida","LC","Carnivore","Temperate","Americas",1,1,1,2},
    {"ORB","OrbWeaver","Golden Silk Orb-Weaver","Nephila clavipes","Arachnida","LC","Carnivore","Tropical","Americas",0,0,1,2},
    {"VIN","Vinegaroon","Giant Vinegaroon","Mastigoproctus giganteus","Arachnida","LC","Carnivore","Arid","Americas",0,1,1,2},
};
static const int NUM_SPECIES = sizeof(SPECIES) / sizeof(SPECIES[0]);

// =============================================================================
// ANIMAL DATA (generated at startup)
// =============================================================================

struct AnimalInfo {
    string id;           // set name, e.g. "TGRF01"
    int species_idx;
    string sex;
    int birth_year;
    bool wild_caught;
    string status;       // Resident / Deceased / TransferredOut / Quarantine / Released
    int enclosure;       // 1..35
    bool in_behav, in_nutri, in_vetsci;  // all are in enviro
};

static vector<AnimalInfo> g_animals;
static vector<string> g_behav, g_nutri, g_vetsci, g_enviro;  // enrollment lists
static map<string, int> g_animal_enclosure;  // animal_id -> enclosure number
static map<string, int> g_animal_idx;        // animal_id -> index in g_animals

// Animals in ENC-07 (needed for week 48 cluster)
static vector<string> g_enc07_animals;

// Observation counters per program
static int g_behav_obs = 0, g_nutri_obs = 0, g_vetsci_obs = 0, g_enviro_obs = 0;

// Medical event tracking
static vector<string> g_medical_events;
static int g_med_fatal = 0, g_med_transferred = 0;

// Diet change tracking
static vector<string> g_diet_changes;

// Diet intervention cohort
static vector<string> g_diet_intervention;

// Pre-save query results for SC-8 persistence check
static map<string, string> g_presave_results;

void generate_animals(mt19937& rng) {
    int idx = 0;
    int enc = 1;
    for (int si = 0; si < NUM_SPECIES; si++) {
        for (int ai = 0; ai < SPECIES[si].animal_count; ai++) {
            AnimalInfo a;
            string sex = (ai % 2 == 0) ? "F" : "M";
            char buf[16];
            snprintf(buf, sizeof(buf), "%s%s%02d", SPECIES[si].code, sex.c_str(), ai / 2 + 1);
            a.id = buf;
            a.species_idx = si;
            a.sex = sex;
            a.birth_year = 2010 + (rng() % 14);
            a.wild_caught = (rng() % 5 == 0);  // 20%

            // Status: ~260 Resident, ~10 Deceased, ~5 TransferredOut, ~3 Quarantine, ~2 Released
            int sr = rng() % 280;
            if (sr < 260) a.status = "Resident";
            else if (sr < 270) a.status = "Deceased";
            else if (sr < 275) a.status = "TransferredOut";
            else if (sr < 278) a.status = "Quarantine";
            else a.status = "Released";

            a.enclosure = enc;
            enc = (enc % 35) + 1;

            // Enrollment: BEHAV first 120, NUTRI first 150, VETSCI indices 30..119
            a.in_behav = (idx < 120);
            a.in_nutri = (idx < 150);
            a.in_vetsci = (idx >= 30 && idx < 120);
            // all in enviro

            g_animal_enclosure[a.id] = a.enclosure;
            g_animal_idx[a.id] = idx;
            g_animals.push_back(a);

            if (a.in_behav) g_behav.push_back(a.id);
            if (a.in_nutri) g_nutri.push_back(a.id);
            if (a.in_vetsci) g_vetsci.push_back(a.id);
            g_enviro.push_back(a.id);

            if (a.enclosure == 7) g_enc07_animals.push_back(a.id);

            idx++;
        }
    }
    cout << "Generated " << g_animals.size() << " animals across " << NUM_SPECIES << " species" << endl;
    cout << "  BEHAV: " << g_behav.size() << ", NUTRI: " << g_nutri.size()
         << ", VETSCI: " << g_vetsci.size() << ", ENVIRO: " << g_enviro.size() << endl;
}

// =============================================================================
// PHASE 1: TAXONOMY
// =============================================================================

void build_taxonomy(cantordb& db) {
    cout << "\n=== PHASE 1: Taxonomy ===" << endl;
    auto t0 = high_resolution_clock::now();

    // Animalia and class sets
    db.create_set("Animalia");
    const char* classes[] = {"Mammalia","Aves","Reptilia","Amphibia","Actinopterygii","Insecta","Arachnida"};
    for (auto& c : classes) {
        db.create_set(c);
        db.add_member("Animalia", c);
    }

    // Trait sets
    const char* traits[] = {"Endangered","Carnivores","Herbivores","Omnivores","Nocturnal",
                            "Venomous","Solitary","AsiaNative","AfricaNative","AmericasNative"};
    for (auto& t : traits) db.create_set(t);

    // Register species properties
    db.create_property("common_name", "string");
    db.create_property("binomial_name", "string");
    db.create_property("iucn_status", "string");
    db.create_property("diet_type", "string");
    db.create_property("native_biome", "string");
    db.create_property("venomous", "bool");
    db.create_property("nocturnal", "bool");
    db.create_property("solitary", "bool");

    // Create species sets with properties and trait memberships
    for (int i = 0; i < NUM_SPECIES; i++) {
        auto& sp = SPECIES[i];
        db.create_set(sp.name);
        db.add_member(sp.cls, sp.name);

        db.add_property("common_name", string(sp.common), sp.name);
        db.add_property("binomial_name", string(sp.binomial), sp.name);
        db.add_property("iucn_status", string(sp.iucn), sp.name);
        db.add_property("diet_type", string(sp.diet), sp.name);
        db.add_property("native_biome", string(sp.biome), sp.name);
        db.add_property("venomous", sp.venomous, sp.name);
        db.add_property("nocturnal", sp.nocturnal, sp.name);
        db.add_property("solitary", sp.solitary, sp.name);

        // Trait memberships
        string iucn(sp.iucn);
        if (iucn == "EN" || iucn == "CR") db.add_member("Endangered", sp.name);

        string diet(sp.diet);
        if (diet == "Carnivore") db.add_member("Carnivores", sp.name);
        else if (diet == "Herbivore") db.add_member("Herbivores", sp.name);
        else if (diet == "Omnivore") db.add_member("Omnivores", sp.name);

        if (sp.nocturnal) db.add_member("Nocturnal", sp.name);
        if (sp.venomous) db.add_member("Venomous", sp.name);
        if (sp.solitary) db.add_member("Solitary", sp.name);

        string region(sp.region);
        if (region == "Asia") db.add_member("AsiaNative", sp.name);
        else if (region == "Africa") db.add_member("AfricaNative", sp.name);
        else if (region == "Americas") db.add_member("AmericasNative", sp.name);
    }

    auto t1 = high_resolution_clock::now();
    long long us = duration_cast<microseconds>(t1 - t0).count();
    log_op("Taxonomy", "Phase 1: Created 7 classes, 80 species, 10 trait sets, properties",
           us, NUM_SPECIES, "Pass");
    cout << "  Taxonomy built in " << us << " us" << endl;
}

// =============================================================================
// PHASE 2: ANIMALS + ENCLOSURES
// =============================================================================

void build_animals(cantordb& db) {
    cout << "\n=== PHASE 2: Animals & Enclosures ===" << endl;
    auto t0 = high_resolution_clock::now();

    // Register animal properties
    db.create_property("animal_id", "string");
    db.create_property("sex", "string");
    db.create_property("birth_year", "int");
    db.create_property("wild_caught", "bool");
    db.create_property("status", "string");
    db.create_property("enclosure_id", "string");

    // Status tracking sets
    const char* statuses[] = {"ResidentAnimals","DeceasedAnimals","TransferredOut",
                              "QuarantineAnimals","ReleasedAnimals"};
    for (auto& s : statuses) db.create_set(s);

    // Enclosure sets (ENC-01 .. ENC-35)
    for (int i = 1; i <= 35; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "ENC-%02d", i);
        db.create_set(buf);
    }

    // Create animals
    for (auto& a : g_animals) {
        db.create_set(a.id);
        db.add_member(SPECIES[a.species_idx].name, a.id);

        db.add_property("animal_id", a.id, a.id);
        db.add_property("sex", a.sex, a.id);
        db.add_property("birth_year", a.birth_year, a.id);
        db.add_property("wild_caught", a.wild_caught, a.id);
        db.add_property("status", a.status, a.id);

        char enc_buf[16];
        snprintf(enc_buf, sizeof(enc_buf), "ENC-%02d", a.enclosure);
        db.add_property("enclosure_id", string(enc_buf), a.id);
        db.add_member(enc_buf, a.id);

        // Status set
        if (a.status == "Resident") db.add_member("ResidentAnimals", a.id);
        else if (a.status == "Deceased") db.add_member("DeceasedAnimals", a.id);
        else if (a.status == "TransferredOut") db.add_member("TransferredOut", a.id);
        else if (a.status == "Quarantine") db.add_member("QuarantineAnimals", a.id);
        else if (a.status == "Released") db.add_member("ReleasedAnimals", a.id);
    }

    auto t1 = high_resolution_clock::now();
    long long us = duration_cast<microseconds>(t1 - t0).count();
    log_op("Animals", "Phase 2: Created 280 animals, 35 enclosures, status sets",
           us, (int)g_animals.size(), "Pass");
    cout << "  " << g_animals.size() << " animals built in " << us << " us" << endl;
}

// =============================================================================
// PHASE 3: ENROLLMENT
// =============================================================================

void build_enrollment(cantordb& db) {
    cout << "\n=== PHASE 3: Research Enrollment ===" << endl;
    auto t0 = high_resolution_clock::now();

    db.create_set("BEHAV_Program");
    db.create_set("NUTRI_Program");
    db.create_set("VETSCI_Program");
    db.create_set("ENVIRO_Program");

    for (auto& id : g_behav) db.add_member("BEHAV_Program", id);
    for (auto& id : g_nutri) db.add_member("NUTRI_Program", id);
    for (auto& id : g_vetsci) db.add_member("VETSCI_Program", id);
    for (auto& id : g_enviro) db.add_member("ENVIRO_Program", id);

    // Cohort sets
    db.create_set("BEHAVandNUTRI_Cohort");
    db.create_set("BEHAVandVETSCI_Cohort");
    db.create_set("AllPrograms_Cohort");

    // BEHAVandNUTRI = first 120 (in both BEHAV [0..119] and NUTRI [0..149])
    for (int i = 0; i < (int)g_animals.size() && i < 120; i++)
        db.add_member("BEHAVandNUTRI_Cohort", g_animals[i].id);

    // BEHAVandVETSCI = indices 30..119 (in both BEHAV [0..119] and VETSCI [30..119])
    for (int i = 30; i < (int)g_animals.size() && i < 120; i++)
        db.add_member("BEHAVandVETSCI_Cohort", g_animals[i].id);

    // AllPrograms = indices 30..119 (in all four: BEHAV, NUTRI, VETSCI, ENVIRO)
    for (int i = 30; i < (int)g_animals.size() && i < 120; i++)
        db.add_member("AllPrograms_Cohort", g_animals[i].id);

    // Per-animal observation archive sets
    for (auto& a : g_animals) {
        db.create_set(a.id + "_Archive");
    }

    // Per-program observation container sets
    db.create_set("BEHAV_Observations");
    db.create_set("NUTRI_Observations");
    db.create_set("VETSCI_Observations");
    db.create_set("ENVIRO_Observations");

    auto t1 = high_resolution_clock::now();
    long long us = duration_cast<microseconds>(t1 - t0).count();
    log_op("Enrollment", "Phase 3: Programs, cohorts, archives, observation containers",
           us, 4, "Pass");
    cout << "  Enrollment built in " << us << " us" << endl;
}

// =============================================================================
// REGISTER OBSERVATION + EVENT PROPERTIES
// =============================================================================

void register_observation_properties(cantordb& db) {
    // BEHAV
    db.create_property("activity_score", "int");
    db.create_property("social_interactions", "int");
    db.create_property("stress_indicator", "string");
    db.create_property("stereotypy_observed", "bool");
    db.create_property("observer_id", "string");
    // NUTRI
    db.create_property("weight_kg", "double");
    db.create_property("diet_code", "string");
    db.create_property("calories_offered", "int");
    db.create_property("calories_consumed", "int");
    db.create_property("diet_change_this_week", "bool");
    // VETSCI
    db.create_property("health_status", "string");
    db.create_property("medical_event", "bool");
    db.create_property("treatment_code", "string");
    db.create_property("exam_conducted", "bool");
    db.create_property("vet_id", "string");
    // ENVIRO
    db.create_property("enclosure_temp_c", "double");
    db.create_property("humidity_pct", "int");
    db.create_property("noise_level", "string");
    db.create_property("enrichment_provided", "bool");
    db.create_property("enclosure_cleaned", "bool");
    // Enclosure conditions (Phase 4)
    db.create_property("avg_temp_c", "double");
    db.create_property("avg_humidity_pct", "int");
    db.create_property("occupancy_count", "int");
    db.create_property("maintenance_event", "bool");
    db.create_property("temp_deviation_flag", "bool");
    // Medical events (Phase 5)
    db.create_property("event_type", "string");
    db.create_property("severity", "string");
    db.create_property("outcome", "string");
    db.create_property("followup_required", "bool");
    db.create_property("resolved_week", "int");
    // Diet changes (Phase 6)
    db.create_property("prior_diet_code", "string");
    db.create_property("new_diet_code", "string");
    db.create_property("reason", "string");
    db.create_property("authorized_by", "string");
}

// =============================================================================
// PHASES 4-7: WEEKLY OBSERVATION LOOP + EVENTS
// =============================================================================

// Helper: generate stress indicator
string rand_stress(mt19937& rng) {
    int r = rng() % 100;
    if (r < 60) return "Low";
    if (r < 85) return "Moderate";
    return "High";
}

string rand_health(mt19937& rng) {
    int r = rng() % 100;
    if (r < 70) return "Healthy";
    if (r < 85) return "Monitoring";
    if (r < 93) return "Ill";
    if (r < 98) return "Recovering";
    return "Critical";
}

string rand_noise(mt19937& rng) {
    int r = rng() % 100;
    if (r < 50) return "Low";
    if (r < 85) return "Moderate";
    return "High";
}

string rand_treatment(mt19937& rng) {
    int r = rng() % 100;
    if (r < 50) return "NONE";
    if (r < 70) return "ANTIBIOTICS";
    if (r < 85) return "ANTIPARASITIC";
    if (r < 95) return "SURGICAL";
    return "SUPPORTIVE";
}

string rand_severity(mt19937& rng) {
    int r = rng() % 100;
    if (r < 40) return "Minor";
    if (r < 70) return "Moderate";
    if (r < 90) return "Severe";
    return "Critical";
}

string rand_outcome(mt19937& rng) {
    // Ensure we get enough Fatal and TransferredCare
    int r = rng() % 100;
    if (r < 55) return "Resolved";
    if (r < 80) return "Ongoing";
    if (r < 92) return "TransferredCare";
    return "Fatal";
}

string rand_event_type(mt19937& rng) {
    const char* types[] = {"Infection","Injury","Parasitic","Respiratory","GI","Dental","Dermal"};
    return types[rng() % 7];
}

string rand_diet_reason(mt19937& rng) {
    const char* reasons[] = {"WeightManagement","NutritionalDeficiency","SeasonalAdjustment",
                             "MedicalRequirement","BehavioralEnrichment"};
    return reasons[rng() % 5];
}

void build_weekly_data(cantordb& db, mt19937& rng) {
    cout << "\n=== PHASES 4-7: Weekly Data Construction ===" << endl;
    auto total_t0 = high_resolution_clock::now();

    register_observation_properties(db);

    // Medical event container sets
    db.create_set("MedicalEvents");
    db.create_set("MedEvents_Resolved");
    db.create_set("MedEvents_Ongoing");
    db.create_set("MedEvents_Fatal");
    db.create_set("MedEvents_TransferredCare");

    // Diet change container
    db.create_set("DietChangeEvents");

    // Temperature deviation tracking set
    db.create_set("TempDeviationWeeks");

    // Track diet codes per animal for NUTRI
    map<string, string> animal_diet;
    for (auto& id : g_nutri) {
        animal_diet[id] = "D0" + to_string(1 + rng() % 5);
    }

    // Track which enclosures are active (ENC-12 shuts down at week 30)
    set<int> active_enclosures;
    for (int i = 1; i <= 35; i++) active_enclosures.insert(i);

    // Week containers for enclosure conditions
    // (observation week containers created per-week below)

    int total_obs_created = 0;
    int total_memberships = 0;
    int total_properties = 0;
    int total_enc_conditions = 0;

    for (int week = 1; week <= 52; week++) {
        auto wk_t0 = high_resolution_clock::now();

        // --- Create week containers ---
        char wk_obs[32], wk_enc[48];
        snprintf(wk_obs, sizeof(wk_obs), "Week%02d_Observations", week);
        snprintf(wk_enc, sizeof(wk_enc), "Week%02d_EnclosureConditions", week);
        db.create_set(wk_obs);
        db.create_set(wk_enc);

        // ================================================================
        // ANNUAL EVENTS (before generating this week's observations)
        // ================================================================

        // Week 6: 15 new VETSCI enrollments + backdated observations
        if (week == 6) {
            cout << "  [Event W06] Enrolling 15 new VETSCI animals + backdating..." << endl;
            // Pick 15 animals not currently in VETSCI (indices 120..134)
            vector<string> new_vetsci;
            for (int i = 120; i < 135 && i < (int)g_animals.size(); i++) {
                new_vetsci.push_back(g_animals[i].id);
                g_vetsci.push_back(g_animals[i].id);
                db.add_member("VETSCI_Program", g_animals[i].id);
            }
            // Backdate: create VETSCI observations for weeks 1-5
            for (int bw = 1; bw <= 5; bw++) {
                char bwk[32];
                snprintf(bwk, sizeof(bwk), "Week%02d_Observations", bw);
                for (auto& aid : new_vetsci) {
                    char obs_name[64];
                    snprintf(obs_name, sizeof(obs_name), "%s_W%02d_V", aid.c_str(), bw);
                    db.create_set(obs_name);
                    db.add_member(bwk, obs_name);
                    db.add_member(aid + "_Archive", obs_name);
                    db.add_member("VETSCI_Observations", obs_name);
                    db.add_property("health_status", string("Healthy"), obs_name);
                    db.add_property("medical_event", false, obs_name);
                    db.add_property("treatment_code", string("NONE"), obs_name);
                    db.add_property("exam_conducted", false, obs_name);
                    char vet[8]; snprintf(vet, sizeof(vet), "VET-%02d", 1 + (int)(rng() % 10));
                    db.add_property("vet_id", string(vet), obs_name);
                    total_obs_created++;
                    total_memberships += 3;
                    total_properties += 5;
                    g_vetsci_obs++;
                }
            }
        }

        // Week 12: Interim save
        if (week == 12) {
            cout << "  [Event W12] Interim research snapshot..." << endl;
            sq(db, "SAVE hzri_week12_research", "Event", "Week 12 interim save");
        }

        // Week 18: Diet intervention
        if (week == 18) {
            cout << "  [Event W18] Diet intervention for 30 NUTRI animals..." << endl;
            db.create_set("DietIntervention_W18");
            g_diet_intervention.clear();
            for (int i = 0; i < 30 && i < (int)g_nutri.size(); i++) {
                db.add_member("DietIntervention_W18", g_nutri[i]);
                g_diet_intervention.push_back(g_nutri[i]);
                animal_diet[g_nutri[i]] = "DNEW";
            }
        }

        // Week 24: BEHAV expansion (+20 animals)
        if (week == 24) {
            cout << "  [Event W24] Adding 20 animals to BEHAV..." << endl;
            // Pick animals 120..139 (some may already be in BEHAV)
            int added = 0;
            for (int i = 120; i < (int)g_animals.size() && added < 20; i++) {
                if (!g_animals[i].in_behav) {
                    g_behav.push_back(g_animals[i].id);
                    g_animals[i].in_behav = true;
                    db.add_member("BEHAV_Program", g_animals[i].id);
                    added++;
                }
            }
        }

        // Week 30: Enclosure renovation — ENC-12 shuts down, animals move to ENC-13
        if (week == 30) {
            cout << "  [Event W30] ENC-12 renovation, animals move to ENC-13..." << endl;
            active_enclosures.erase(12);
            // Move animals from ENC-12 to ENC-13
            for (auto& a : g_animals) {
                if (a.enclosure == 12) {
                    db.remove_member("ENC-12", a.id);
                    db.add_member("ENC-13", a.id);
                    db.update_property(a.id, "enclosure_id", string("ENC-13"));
                    a.enclosure = 13;
                    g_animal_enclosure[a.id] = 13;
                }
            }
        }

        // Week 35: 20 animals complete VETSCI protocol
        if (week == 35) {
            cout << "  [Event W35] Removing 20 animals from VETSCI..." << endl;
            int removed = 0;
            auto it = g_vetsci.begin();
            while (it != g_vetsci.end() && removed < 20) {
                db.remove_member("VETSCI_Program", *it);
                it = g_vetsci.erase(it);
                removed++;
            }
        }

        // Week 40: Mid-year save
        if (week == 40) {
            cout << "  [Event W40] Mid-year research snapshot..." << endl;
            sq(db, "SAVE hzri_week40_research", "Event", "Week 40 mid-year save");
        }

        // Week 48: Health cluster in ENC-07
        if (week == 48) {
            cout << "  [Event W48] Critical health cluster in ENC-07..." << endl;
            db.create_set("HealthCluster_W48_ENC07");
            int cluster_count = min(4, (int)g_enc07_animals.size());
            for (int ci = 0; ci < cluster_count; ci++) {
                string aid = g_enc07_animals[ci];
                char med_name[64];
                snprintf(med_name, sizeof(med_name), "Med_%s_W48", aid.c_str());
                db.create_set(med_name);
                db.add_member("MedicalEvents", med_name);
                db.add_member(aid + "_Archive", med_name);
                db.add_member("HealthCluster_W48_ENC07", med_name);
                db.add_property("event_type", string("Respiratory"), med_name);
                db.add_property("severity", string("Severe"), med_name);
                db.add_property("outcome", string("Ongoing"), med_name);
                db.add_property("followup_required", true, med_name);
                db.add_property("resolved_week", 0, med_name);
                db.add_member("MedEvents_Ongoing", med_name);
                g_medical_events.push_back(med_name);
            }
        }

        // ================================================================
        // GENERATE THIS WEEK'S OBSERVATIONS
        // ================================================================

        // BEHAV observations
        for (auto& aid : g_behav) {
            char obs[64];
            snprintf(obs, sizeof(obs), "%s_W%02d_B", aid.c_str(), week);
            db.create_set(obs);
            db.add_member(wk_obs, obs);
            db.add_member(aid + "_Archive", obs);
            db.add_member("BEHAV_Observations", obs);
            total_memberships += 3;

            int act = 1 + rng() % 10;
            int soc = rng() % 16;
            string stress = rand_stress(rng);
            bool stereo = (rng() % 10 == 0);
            char obs_id[16]; snprintf(obs_id, sizeof(obs_id), "OBS-%02d", 1 + (int)(rng() % 10));

            db.add_property("activity_score", act, obs);
            db.add_property("social_interactions", soc, obs);
            db.add_property("stress_indicator", stress, obs);
            db.add_property("stereotypy_observed", stereo, obs);
            db.add_property("observer_id", string(obs_id), obs);
            total_properties += 5;
            total_obs_created++;
            g_behav_obs++;
        }

        // NUTRI observations
        for (auto& aid : g_nutri) {
            char obs[64];
            snprintf(obs, sizeof(obs), "%s_W%02d_N", aid.c_str(), week);
            db.create_set(obs);
            db.add_member(wk_obs, obs);
            db.add_member(aid + "_Archive", obs);
            db.add_member("NUTRI_Observations", obs);
            total_memberships += 3;

            double weight = 10.0 + (rng() % 500) / 10.0;  // 10-60 kg range
            string dc = animal_diet.count(aid) ? animal_diet[aid] : "D01";
            int cal_offered = 500 + rng() % 4500;
            int cal_consumed = (int)(cal_offered * (0.5 + (rng() % 50) / 100.0));
            // ~2.5% chance of diet change
            bool diet_change = (rng() % 40 == 0);

            db.add_property("weight_kg", weight, obs);
            db.add_property("diet_code", dc, obs);
            db.add_property("calories_offered", cal_offered, obs);
            db.add_property("calories_consumed", cal_consumed, obs);
            db.add_property("diet_change_this_week", diet_change, obs);
            total_properties += 5;
            total_obs_created++;
            g_nutri_obs++;

            // Diet change event
            if (diet_change && (int)g_diet_changes.size() < 220) {
                char dc_name[64];
                snprintf(dc_name, sizeof(dc_name), "Diet_%s_W%02d", aid.c_str(), week);
                db.create_set(dc_name);
                db.add_member("DietChangeEvents", dc_name);
                db.add_member(aid + "_Archive", dc_name);
                string old_dc = dc;
                string new_dc = "D0" + to_string(1 + rng() % 5);
                db.add_property("prior_diet_code", old_dc, dc_name);
                db.add_property("new_diet_code", new_dc, dc_name);
                db.add_property("reason", rand_diet_reason(rng), dc_name);
                char auth[16]; snprintf(auth, sizeof(auth), "NUTR-%02d", 1 + (int)(rng() % 5));
                db.add_property("authorized_by", string(auth), dc_name);
                g_diet_changes.push_back(dc_name);
                animal_diet[aid] = new_dc;
            }
        }

        // VETSCI observations
        for (auto& aid : g_vetsci) {
            char obs[64];
            snprintf(obs, sizeof(obs), "%s_W%02d_V", aid.c_str(), week);
            db.create_set(obs);
            db.add_member(wk_obs, obs);
            db.add_member(aid + "_Archive", obs);
            db.add_member("VETSCI_Observations", obs);
            total_memberships += 3;

            string hs = rand_health(rng);
            bool med_ev = (rng() % 33 == 0);  // ~3%
            string tx = med_ev ? rand_treatment(rng) : string("NONE");
            bool exam = (rng() % 5 == 0);
            char vet[8]; snprintf(vet, sizeof(vet), "VET-%02d", 1 + (int)(rng() % 10));

            db.add_property("health_status", hs, obs);
            db.add_property("medical_event", med_ev, obs);
            db.add_property("treatment_code", tx, obs);
            db.add_property("exam_conducted", exam, obs);
            db.add_property("vet_id", string(vet), obs);
            total_properties += 5;
            total_obs_created++;
            g_vetsci_obs++;

            // Medical event detail set
            if (med_ev && (int)g_medical_events.size() < 155) {
                char med_name[64];
                snprintf(med_name, sizeof(med_name), "Med_%s_W%02d", aid.c_str(), week);
                db.create_set(med_name);
                db.add_member("MedicalEvents", med_name);
                db.add_member(aid + "_Archive", med_name);

                string sev = rand_severity(rng);
                string out;
                // Force some Fatal and TransferredCare to meet minimums
                if (g_med_fatal < 3 && (int)g_medical_events.size() > 100) out = "Fatal";
                else if (g_med_transferred < 5 && (int)g_medical_events.size() > 80) out = "TransferredCare";
                else out = rand_outcome(rng);

                if (out == "Fatal") g_med_fatal++;
                if (out == "TransferredCare") g_med_transferred++;

                db.add_property("event_type", rand_event_type(rng), med_name);
                db.add_property("severity", sev, med_name);
                db.add_property("outcome", out, med_name);
                db.add_property("followup_required", (sev == "Severe" || sev == "Critical"), med_name);
                db.add_property("resolved_week", (out == "Resolved") ? week + (int)(rng() % 4) : 0, med_name);

                // Outcome grouping
                if (out == "Resolved") db.add_member("MedEvents_Resolved", med_name);
                else if (out == "Ongoing") db.add_member("MedEvents_Ongoing", med_name);
                else if (out == "Fatal") db.add_member("MedEvents_Fatal", med_name);
                else if (out == "TransferredCare") db.add_member("MedEvents_TransferredCare", med_name);

                g_medical_events.push_back(med_name);
            }
        }

        // ENVIRO observations (all 280 animals)
        for (auto& aid : g_enviro) {
            char obs[64];
            snprintf(obs, sizeof(obs), "%s_W%02d_E", aid.c_str(), week);
            db.create_set(obs);
            db.add_member(wk_obs, obs);
            db.add_member(aid + "_Archive", obs);
            db.add_member("ENVIRO_Observations", obs);
            total_memberships += 3;

            double temp = 15.0 + (rng() % 200) / 10.0;  // 15-35
            int hum = 30 + rng() % 61;                    // 30-90
            string noise = rand_noise(rng);
            bool enrichment = (rng() % 5 != 0);  // 80%
            bool cleaned = (rng() % 10 < 7);     // 70%

            db.add_property("enclosure_temp_c", temp, obs);
            db.add_property("humidity_pct", hum, obs);
            db.add_property("noise_level", noise, obs);
            db.add_property("enrichment_provided", enrichment, obs);
            db.add_property("enclosure_cleaned", cleaned, obs);
            total_properties += 5;
            total_obs_created++;
            g_enviro_obs++;
        }

        // ENCLOSURE CONDITIONS (Phase 4)
        for (int enc : active_enclosures) {
            char cond_name[48];
            snprintf(cond_name, sizeof(cond_name), "ENC%02d_W%02d_Cond", enc, week);
            char enc_name[16];
            snprintf(enc_name, sizeof(enc_name), "ENC-%02d", enc);

            db.create_set(cond_name);
            db.add_member(enc_name, cond_name);
            db.add_member(wk_enc, cond_name);
            total_memberships += 2;

            double avg_t = 15.0 + (rng() % 200) / 10.0;
            int avg_h = 30 + rng() % 61;
            int occ = db.get_cardinality(enc_name);  // animals + conditions
            bool maint = (rng() % 20 == 0);
            bool temp_dev = (rng() % 12 == 0);  // ~8%

            db.add_property("avg_temp_c", avg_t, cond_name);
            db.add_property("avg_humidity_pct", avg_h, cond_name);
            db.add_property("occupancy_count", occ, cond_name);
            db.add_property("maintenance_event", maint, cond_name);
            db.add_property("temp_deviation_flag", temp_dev, cond_name);
            total_properties += 5;
            total_enc_conditions++;

            if (temp_dev) db.add_member("TempDeviationWeeks", cond_name);
        }

        // Week 52: Full year save
        if (week == 52) {
            cout << "  [Event W52] Full year save..." << endl;
            sq(db, "SAVE hzri_fullyear_research", "Event", "Week 52 full year save");
        }

        auto wk_t1 = high_resolution_clock::now();
        long long wk_us = duration_cast<microseconds>(wk_t1 - wk_t0).count();

        // Log per-week aggregate
        int wk_obs_count = (int)(g_behav.size() + g_nutri.size() + g_vetsci.size() + g_enviro.size());
        char wk_desc[128];
        snprintf(wk_desc, sizeof(wk_desc),
                 "Week %02d: %d obs + %d enc conditions (%d memberships, %d properties)",
                 week, wk_obs_count, (int)active_enclosures.size(),
                 wk_obs_count * 3 + (int)active_enclosures.size() * 2,
                 wk_obs_count * 5 + (int)active_enclosures.size() * 5);
        log_op("Construction", wk_desc, wk_us, wk_obs_count, "Pass");

        if (week % 10 == 0 || week == 1 || week == 52) {
            cout << "  Week " << week << "/52 done (" << wk_us << " us)" << endl;
        }
    }

    auto total_t1 = high_resolution_clock::now();
    long long total_us = duration_cast<microseconds>(total_t1 - total_t0).count();

    cout << "\n  --- Construction Summary ---" << endl;
    cout << "  Total observation sets: " << total_obs_created << endl;
    cout << "    BEHAV: " << g_behav_obs << "  NUTRI: " << g_nutri_obs
         << "  VETSCI: " << g_vetsci_obs << "  ENVIRO: " << g_enviro_obs << endl;
    cout << "  Total memberships: " << total_memberships << endl;
    cout << "  Total properties: " << total_properties << endl;
    cout << "  Enclosure conditions: " << total_enc_conditions << endl;
    cout << "  Medical events: " << g_medical_events.size()
         << " (Fatal: " << g_med_fatal << ", TransferredCare: " << g_med_transferred << ")" << endl;
    cout << "  Diet changes: " << g_diet_changes.size() << endl;
    cout << "  Total construction time: " << total_us / 1000 << " ms" << endl;

    log_op("Construction", "Phases 4-7 complete", total_us, total_obs_created, "Pass",
           "Total memberships: " + to_string(total_memberships) +
           ", Total properties: " + to_string(total_properties));
}

// =============================================================================
// QUERIES: TAXONOMY AND ENROLLMENT (Q-BASE)
// =============================================================================

void run_base_queries(cantordb& db) {
    cout << "\n=== Q-BASE: Taxonomy & Enrollment Queries ===" << endl;

    // Q-BASE-1
    sq(db, "GET ELEMENTS OF BEHAV_Program", "Q-BASE", "Q-BASE-1: All BEHAV animals");

    // Q-BASE-2
    sq(db, "GET ELEMENTS OF BEHAV_Program INTERSECTION NUTRI_Program", "Q-BASE",
       "Q-BASE-2: BEHAV AND NUTRI intersection");

    // Q-BASE-3
    sq(db, "GET ELEMENTS OF BEHAV_Program DIFFERENCE VETSCI_Program", "Q-BASE",
       "Q-BASE-3: BEHAV minus VETSCI");

    // Q-BASE-4
    sq(db, "GET ELEMENTS OF BEHAV_Program UNION NUTRI_Program UNION VETSCI_Program UNION ENVIRO_Program",
       "Q-BASE", "Q-BASE-4: Union of all programs");

    // Q-BASE-5: Endangered intersection — species are in Endangered, not animals
    string r5 = sq(db, "GET ELEMENTS OF VETSCI_Program INTERSECTION Endangered", "Q-BASE",
       "Q-BASE-5: VETSCI INTERSECTION Endangered (expects empty — species-level gap)");
    if (count_lines(r5) == 0) {
        log_op("Q-BASE", "GAP: Membership traversal needed — Endangered holds species not animals",
               0, 0, "API-Fallback",
               "SeQL cannot traverse species->animal membership to infer animal-level trait membership");
    }

    // Q-BASE-6
    sq(db, "GET CARDINALITY OF BEHAV_Program", "Q-BASE", "Q-BASE-6a: BEHAV count");
    sq(db, "GET CARDINALITY OF NUTRI_Program", "Q-BASE", "Q-BASE-6b: NUTRI count");
    sq(db, "GET CARDINALITY OF VETSCI_Program", "Q-BASE", "Q-BASE-6c: VETSCI count");
    sq(db, "GET CARDINALITY OF ENVIRO_Program", "Q-BASE", "Q-BASE-6d: ENVIRO count");

    // Q-BASE-7
    sq(db, "GET ELEMENTS OF AllPrograms_Cohort", "Q-BASE", "Q-BASE-7: All-programs cohort");
}

// =============================================================================
// QUERIES: BEHAVIORAL STUDY (Q-BEHAV)
// =============================================================================

void run_behav_queries(cantordb& db) {
    cout << "\n=== Q-BEHAV: Behavioral Study Queries ===" << endl;

    // Q-BEHAV-1
    sq(db, "GET ELEMENTS OF BEHAV_Observations WHERE stress_indicator = High", "Q-BEHAV",
       "Q-BEHAV-1: High stress observations");

    // Q-BEHAV-2
    sq(db, "GET ELEMENTS OF BEHAV_Observations WHERE stereotypy_observed = true", "Q-BEHAV",
       "Q-BEHAV-2: Stereotypy observations");

    // Q-BEHAV-3
    sq(db, "GET ELEMENTS OF BEHAV_Observations WHERE stress_indicator = High AND stereotypy_observed = true",
       "Q-BEHAV", "Q-BEHAV-3: High stress AND stereotypy");

    // Q-BEHAV-4
    sq(db, "GET ELEMENTS OF BEHAV_Observations WHERE activity_score < 3", "Q-BEHAV",
       "Q-BEHAV-4: Low activity (score < 3)");

    // Q-BEHAV-5
    sq(db, "GET ELEMENTS OF BEHAV_Observations WHERE activity_score < 3 OR stress_indicator = High",
       "Q-BEHAV", "Q-BEHAV-5: Low activity OR high stress");

    // Q-BEHAV-6: Single animal full-year BEHAV observations
    string first_behav = g_behav.empty() ? "TGRF01" : g_behav[0];
    sq(db, "GET ELEMENTS OF " + first_behav + "_Archive INTERSECTION BEHAV_Observations",
       "Q-BEHAV", "Q-BEHAV-6: Single animal full-year BEHAV");

    // Q-BEHAV-7: Single week
    sq(db, "GET ELEMENTS OF Week20_Observations INTERSECTION BEHAV_Observations",
       "Q-BEHAV", "Q-BEHAV-7: Week 20 BEHAV observations");

    // Q-BEHAV-8: Nocturnal animals — API FALLBACK
    {
        cout << "  Q-BEHAV-8: BEHAV for nocturnal animals (API fallback)..." << endl;
        auto t0 = high_resolution_clock::now();
        // Get nocturnal species
        vector<string> nocturnal_species = to_vec(db.list_elements("Nocturnal"));
        // For each nocturnal species, get its animals, collect their BEHAV archive obs
        int count = 0;
        for (auto& sp : nocturnal_species) {
            vector<string> animals = to_vec(db.list_elements(sp));
            for (auto& aid : animals) {
                // Check if this animal is in BEHAV
                if (db.is_element(aid, "BEHAV_Program")) {
                    Set* result = db.set_intersection(aid + "_Archive", "BEHAV_Observations");
                    if (result) count += (int)result->has_element.size();
                }
            }
        }
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        log_op("Q-BEHAV", "API: Nocturnal species -> animals -> BEHAV obs (multi-level traversal)",
               us, count, "API-Fallback",
               "Requires species->animal membership traversal not expressible in SeQL");
        db.clear_cache();
    }

    // Q-BEHAV-9: Weekly stress trend — API FALLBACK (no ORDER in SeQL)
    {
        cout << "  Q-BEHAV-9: Weekly stress trend for single animal (API fallback)..." << endl;
        auto t0 = high_resolution_clock::now();
        string aid = g_behav.empty() ? "TGRF01" : g_behav[0];
        vector<pair<int, string>> trend;
        for (int w = 1; w <= 52; w++) {
            char obs[64];
            snprintf(obs, sizeof(obs), "%s_W%02d_B", aid.c_str(), w);
            string si = db.get_property_safe_string(obs, "stress_indicator");
            if (!si.empty()) trend.push_back({w, si});
        }
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        log_op("Q-BEHAV", "API: Weekly stress trend for " + aid + " (ordered retrieval)",
               us, (int)trend.size(), "API-Fallback", "No ORDER support in SeQL");
    }

    // Q-BEHAV-10: Count of high-stress per week — API FALLBACK
    {
        cout << "  Q-BEHAV-10: High-stress counts per week (API fallback)..." << endl;
        auto t0 = high_resolution_clock::now();
        vector<int> weekly_counts(53, 0);
        for (int w = 1; w <= 52; w++) {
            char wk[32];
            snprintf(wk, sizeof(wk), "Week%02d_Observations", w);
            Set* inter = db.set_intersection(wk, "BEHAV_Observations");
            if (inter) {
                for (auto* elem : inter->has_element) {
                    string si = db.get_property_safe_string(elem->set_name, "stress_indicator");
                    if (si == "High") weekly_counts[w]++;
                }
            }
        }
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        int total_high = 0;
        for (int w = 1; w <= 52; w++) total_high += weekly_counts[w];
        log_op("Q-BEHAV", "API: Per-week high-stress count across 52 weeks",
               us, total_high, "API-Fallback", "Per-week aggregation not expressible in SeQL");
        db.clear_cache();
    }
}

// =============================================================================
// QUERIES: NUTRITIONAL STUDY (Q-NUTRI)
// =============================================================================

void run_nutri_queries(cantordb& db) {
    cout << "\n=== Q-NUTRI: Nutritional Study Queries ===" << endl;

    // Q-NUTRI-1
    sq(db, "GET ELEMENTS OF NUTRI_Observations WHERE diet_change_this_week = true", "Q-NUTRI",
       "Q-NUTRI-1: Diet changes this week");

    // Q-NUTRI-2
    sq(db, "GET ELEMENTS OF NUTRI_Observations WHERE calories_consumed < 500", "Q-NUTRI",
       "Q-NUTRI-2: Undernourishment (cal < 500)");

    // Q-NUTRI-3
    sq(db, "GET ELEMENTS OF NUTRI_Observations WHERE diet_change_this_week = true AND calories_consumed < 500",
       "Q-NUTRI", "Q-NUTRI-3: Diet change AND low consumption");

    // Q-NUTRI-4: Single animal
    string first_nutri = g_nutri.empty() ? "TGRF01" : g_nutri[0];
    sq(db, "GET ELEMENTS OF " + first_nutri + "_Archive INTERSECTION NUTRI_Observations",
       "Q-NUTRI", "Q-NUTRI-4: Single animal NUTRI observations");

    // Q-NUTRI-5
    sq(db, "GET ELEMENTS OF DietChangeEvents", "Q-NUTRI", "Q-NUTRI-5: All diet change events");

    // Q-NUTRI-6: Diet intervention animals' NUTRI obs (Partial API hybrid)
    {
        cout << "  Q-NUTRI-6: Intervention cohort NUTRI obs (partial hybrid)..." << endl;
        auto t0 = high_resolution_clock::now();
        // SeQL: get intervention animals
        string intervention_animals = parse_query(db, "GET ELEMENTS OF DietIntervention_W18");
        vector<string> int_animals = to_vec(intervention_animals);
        int total_obs = 0;
        // API: per-animal intersection
        for (auto& aid : int_animals) {
            Set* r = db.set_intersection(aid + "_Archive", "NUTRI_Observations");
            if (r) {
                // Filter to week >= 18
                for (auto* elem : r->has_element) {
                    // Parse week from name: <aid>_W<nn>_N
                    string n = elem->set_name;
                    size_t wp = n.find("_W");
                    if (wp != string::npos) {
                        int wn = atoi(n.substr(wp + 2, 2).c_str());
                        if (wn >= 18) total_obs++;
                    }
                }
            }
        }
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        log_op("Q-NUTRI", "Hybrid: DietIntervention_W18 -> per-animal NUTRI obs (week>=18)",
               us, total_obs, "Partial-Hybrid",
               "SeQL handles cohort lookup, API iterates + filters by week");
        db.clear_cache();
    }

    // Q-NUTRI-7: Weight trend for single animal — API FALLBACK
    {
        cout << "  Q-NUTRI-7: Weight trend for single animal (API fallback)..." << endl;
        auto t0 = high_resolution_clock::now();
        string aid = g_nutri.empty() ? "TGRF01" : g_nutri[0];
        vector<pair<int, double>> weights;
        for (int w = 1; w <= 52; w++) {
            char obs[64];
            snprintf(obs, sizeof(obs), "%s_W%02d_N", aid.c_str(), w);
            double wt = db.get_property_safe_double(obs, "weight_kg");
            if (wt > 0.0) weights.push_back({w, wt});
        }
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        log_op("Q-NUTRI", "API: Weight trend for " + aid + " across 52 weeks",
               us, (int)weights.size(), "API-Fallback",
               "No temporal ordering or aggregation in SeQL");
    }

    // Q-NUTRI-8: Avg weight change pre/post intervention — API FALLBACK
    {
        cout << "  Q-NUTRI-8: Avg weight change pre/post intervention (API fallback)..." << endl;
        auto t0 = high_resolution_clock::now();
        double total_change = 0.0;
        int count = 0;
        for (auto& aid : g_diet_intervention) {
            char pre_obs[64], post_obs[64];
            snprintf(pre_obs, sizeof(pre_obs), "%s_W17_N", aid.c_str());
            snprintf(post_obs, sizeof(post_obs), "%s_W30_N", aid.c_str());
            double pre_w = db.get_property_safe_double(pre_obs, "weight_kg");
            double post_w = db.get_property_safe_double(post_obs, "weight_kg");
            if (pre_w > 0.0 && post_w > 0.0) {
                total_change += (post_w - pre_w);
                count++;
            }
        }
        double avg_change = count > 0 ? total_change / count : 0.0;
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        char note[128];
        snprintf(note, sizeof(note), "Cross-observation arithmetic: avg change = %.2f kg over %d animals",
                 avg_change, count);
        log_op("Q-NUTRI", "API: Pre/post intervention weight comparison (W17 vs W30)",
               us, count, "API-Fallback", note);
    }

    // Q-NUTRI-9: Animals with at least one low-consumption obs — API FALLBACK
    {
        cout << "  Q-NUTRI-9: Animals with any low-consumption obs (API fallback)..." << endl;
        auto t0 = high_resolution_clock::now();
        int animals_with_low = 0;
        for (auto& aid : g_nutri) {
            bool found = false;
            for (int w = 1; w <= 52 && !found; w++) {
                char obs[64];
                snprintf(obs, sizeof(obs), "%s_W%02d_N", aid.c_str(), w);
                int cal = db.get_property_safe_int(obs, "calories_consumed");
                if (cal > 0 && cal < 500) found = true;
            }
            if (found) animals_with_low++;
        }
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        log_op("Q-NUTRI", "API: Animals with >=1 low-consumption NUTRI obs",
               us, animals_with_low, "API-Fallback",
               "Membership existence check across archive not expressible in one SeQL query");
    }
}

// =============================================================================
// QUERIES: VETERINARY SCIENCE (Q-VETSCI)
// =============================================================================

void run_vetsci_queries(cantordb& db) {
    cout << "\n=== Q-VETSCI: Veterinary Science Queries ===" << endl;

    // Q-VETSCI-1
    sq(db, "GET ELEMENTS OF VETSCI_Observations WHERE medical_event = true", "Q-VETSCI",
       "Q-VETSCI-1: Medical events");

    // Q-VETSCI-2
    sq(db, "GET ELEMENTS OF VETSCI_Observations WHERE health_status = Ill OR health_status = Critical",
       "Q-VETSCI", "Q-VETSCI-2: Ill or Critical");

    // Q-VETSCI-3
    sq(db, "GET ELEMENTS OF VETSCI_Observations WHERE treatment_code = SURGICAL", "Q-VETSCI",
       "Q-VETSCI-3: Surgical treatments");

    // Q-VETSCI-4
    sq(db, "GET ELEMENTS OF MedEvents_Fatal", "Q-VETSCI", "Q-VETSCI-4: Fatal events");

    // Q-VETSCI-5
    sq(db, "GET ELEMENTS OF MedicalEvents WHERE followup_required = true", "Q-VETSCI",
       "Q-VETSCI-5: Events requiring followup");

    // Q-VETSCI-6
    sq(db, "GET ELEMENTS OF MedicalEvents WHERE severity = Severe OR severity = Critical",
       "Q-VETSCI", "Q-VETSCI-6: Severe or Critical events");

    // Q-VETSCI-7: Animals with medical events — API FALLBACK
    {
        cout << "  Q-VETSCI-7: Animals with medical events (API fallback)..." << endl;
        auto t0 = high_resolution_clock::now();
        set<string> animals_with_events;
        vector<string> events = to_vec(db.list_elements("MedicalEvents"));
        for (auto& ev : events) {
            // Medical event set is member of animal's archive
            // Get parent sets of the event
            vector<string> parents = to_vec(db.list_sets(ev));
            for (auto& p : parents) {
                // Archive sets end with _Archive
                if (p.size() > 8 && p.substr(p.size() - 8) == "_Archive") {
                    string aid = p.substr(0, p.size() - 8);
                    animals_with_events.insert(aid);
                }
            }
        }
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        log_op("Q-VETSCI", "API: Animals with medical events (inverted membership traversal)",
               us, (int)animals_with_events.size(), "API-Fallback",
               "Cannot invert membership direction efficiently in SeQL");
        db.clear_cache();
    }

    // Q-VETSCI-8
    sq(db, "GET ELEMENTS OF HealthCluster_W48_ENC07", "Q-VETSCI",
       "Q-VETSCI-8: Week 48 health cluster");

    // Q-VETSCI-9: Medical events in ENC-07 — API FALLBACK
    {
        cout << "  Q-VETSCI-9: Medical events in ENC-07 (API fallback)..." << endl;
        auto t0 = high_resolution_clock::now();
        int count = 0;
        vector<string> enc07 = to_vec(db.list_elements("ENC-07"));
        set<string> enc07_animals(enc07.begin(), enc07.end());
        vector<string> all_events = to_vec(db.list_elements("MedicalEvents"));
        for (auto& ev : all_events) {
            vector<string> parents = to_vec(db.list_sets(ev));
            for (auto& p : parents) {
                if (p.size() > 8 && p.substr(p.size() - 8) == "_Archive") {
                    string aid = p.substr(0, p.size() - 8);
                    if (enc07_animals.count(aid)) { count++; break; }
                }
            }
        }
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        log_op("Q-VETSCI", "API: Medical events for ENC-07 animals (multi-hop traversal)",
               us, count, "API-Fallback",
               "Requires event->archive->animal->enclosure traversal");
    }

    // Q-VETSCI-10: Health status progression — API FALLBACK
    {
        cout << "  Q-VETSCI-10: Health progression for single animal (API fallback)..." << endl;
        auto t0 = high_resolution_clock::now();
        string aid = g_vetsci.empty() ? "EASF01" : g_vetsci[0];
        vector<pair<int, string>> progression;
        for (int w = 1; w <= 52; w++) {
            char obs[64];
            snprintf(obs, sizeof(obs), "%s_W%02d_V", aid.c_str(), w);
            string hs = db.get_property_safe_string(obs, "health_status");
            if (!hs.empty()) progression.push_back({w, hs});
        }
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        log_op("Q-VETSCI", "API: Health progression for " + aid + " across 52 weeks",
               us, (int)progression.size(), "API-Fallback", "No ORDER in SeQL");
    }

    // Q-VETSCI-11: Diet intervention animals with medical events
    // Test dot notation precedence
    sq(db, "GET ELEMENTS OF DietIntervention_W18 INTERSECTION .MedEvents_Resolved UNION MedEvents_Ongoing UNION MedEvents_Fatal.",
       "Q-VETSCI", "Q-VETSCI-11: Intervention animals with any medical event (dot notation test)");
}

// =============================================================================
// QUERIES: ENVIRONMENTAL IMPACT (Q-ENVIRO)
// =============================================================================

void run_enviro_queries(cantordb& db) {
    cout << "\n=== Q-ENVIRO: Environmental Impact Queries ===" << endl;

    // Q-ENVIRO-1
    sq(db, "GET ELEMENTS OF ENVIRO_Observations WHERE enrichment_provided = false", "Q-ENVIRO",
       "Q-ENVIRO-1: No enrichment");

    // Q-ENVIRO-2
    sq(db, "GET ELEMENTS OF ENVIRO_Observations WHERE noise_level = High", "Q-ENVIRO",
       "Q-ENVIRO-2: High noise");

    // Q-ENVIRO-3
    sq(db, "GET ELEMENTS OF ENVIRO_Observations WHERE noise_level = High AND enrichment_provided = false",
       "Q-ENVIRO", "Q-ENVIRO-3: High noise AND no enrichment");

    // Q-ENVIRO-4: Enclosures with temp deviation — using the aggregation set
    sq(db, "GET ELEMENTS OF TempDeviationWeeks", "Q-ENVIRO",
       "Q-ENVIRO-4: All enclosure-weeks with temp deviation (via aggregation set)");

    // Q-ENVIRO-5: Mixed-type set members
    sq(db, "GET ELEMENTS OF ENC-04", "Q-ENVIRO",
       "Q-ENVIRO-5: All members of ENC-04 (animals + condition sets)");

    // Q-ENVIRO-6: Temp deviation vs stress correlation — API FALLBACK
    {
        cout << "  Q-ENVIRO-6: Temp deviation vs stress correlation (API fallback)..." << endl;
        auto t0 = high_resolution_clock::now();
        int deviation_high_stress = 0, deviation_total = 0;
        int normal_high_stress = 0, normal_total = 0;

        for (int w = 1; w <= 52; w++) {
            // Get enclosures with temp deviation this week
            set<int> deviated_encs;
            for (int enc = 1; enc <= 35; enc++) {
                char cond[48];
                snprintf(cond, sizeof(cond), "ENC%02d_W%02d_Cond", enc, w);
                bool dev = db.get_property_safe_bool(cond, "temp_deviation_flag");
                if (dev) deviated_encs.insert(enc);
            }

            // For BEHAV animals, check if their enclosure had deviation
            for (auto& aid : g_behav) {
                int enc = g_animal_enclosure[aid];
                char bobs[64];
                snprintf(bobs, sizeof(bobs), "%s_W%02d_B", aid.c_str(), w);
                string si = db.get_property_safe_string(bobs, "stress_indicator");
                if (si.empty()) continue;

                if (deviated_encs.count(enc)) {
                    deviation_total++;
                    if (si == "High") deviation_high_stress++;
                } else {
                    normal_total++;
                    if (si == "High") normal_high_stress++;
                }
            }
        }
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        char note[256];
        snprintf(note, sizeof(note),
                 "Deviation weeks: %d/%d high stress (%.1f%%), Normal weeks: %d/%d (%.1f%%)",
                 deviation_high_stress, deviation_total,
                 deviation_total > 0 ? 100.0 * deviation_high_stress / deviation_total : 0.0,
                 normal_high_stress, normal_total,
                 normal_total > 0 ? 100.0 * normal_high_stress / normal_total : 0.0);
        log_op("Q-ENVIRO", "API: Temp deviation vs stress correlation (52 weeks x enclosures)",
               us, deviation_total + normal_total, "API-Fallback", note);
    }

    // Q-ENVIRO-7: ENC-07 ENVIRO observations during W48 cluster period
    {
        cout << "  Q-ENVIRO-7: ENC-07 ENVIRO obs weeks 45-52 (partial hybrid)..." << endl;
        auto t0 = high_resolution_clock::now();
        // SeQL: get ENC-07 animals that are in ENVIRO
        string enc07_enviro = parse_query(db, "GET ELEMENTS OF ENC-07 INTERSECTION ENVIRO_Program");
        vector<string> animals = to_vec(enc07_enviro);
        int count = 0;
        for (auto& aid : animals) {
            for (int w = 45; w <= 52; w++) {
                char obs[64];
                snprintf(obs, sizeof(obs), "%s_W%02d_E", aid.c_str(), w);
                string nl = db.get_property_safe_string(obs, "noise_level");
                if (!nl.empty()) count++;
            }
        }
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        log_op("Q-ENVIRO", "Hybrid: ENC-07 ENVIRO obs weeks 45-52 (SeQL cohort + API temporal filter)",
               us, count, "Partial-Hybrid", "SeQL identifies cohort, API filters by week range");
        db.clear_cache();
    }
}

// =============================================================================
// QUERIES: CROSS-STUDY RESEARCH (Q-CROSS)
// =============================================================================

void run_cross_queries(cantordb& db) {
    cout << "\n=== Q-CROSS: Cross-Study Research Queries ===" << endl;

    // Q-CROSS-1: BEHAV+VETSCI animals with high stress AND medical event
    {
        cout << "  Q-CROSS-1: High-stress + medical event animals (partial hybrid)..." << endl;
        auto t0 = high_resolution_clock::now();
        // Step 1 SeQL
        string cohort = parse_query(db, "GET ELEMENTS OF BEHAV_Program INTERSECTION VETSCI_Program");
        vector<string> animals = to_vec(cohort);
        int match = 0;
        for (auto& aid : animals) {
            bool has_high_stress = false;
            bool has_med_event = false;
            for (int w = 1; w <= 52 && (!has_high_stress || !has_med_event); w++) {
                if (!has_high_stress) {
                    char bobs[64];
                    snprintf(bobs, sizeof(bobs), "%s_W%02d_B", aid.c_str(), w);
                    if (db.get_property_safe_string(bobs, "stress_indicator") == "High")
                        has_high_stress = true;
                }
                if (!has_med_event) {
                    char vobs[64];
                    snprintf(vobs, sizeof(vobs), "%s_W%02d_V", aid.c_str(), w);
                    if (db.get_property_safe_bool(vobs, "medical_event"))
                        has_med_event = true;
                }
            }
            if (has_high_stress && has_med_event) match++;
        }
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        log_op("Q-CROSS", "Hybrid: BEHAV+VETSCI animals with high stress AND medical event",
               us, match, "Partial-Hybrid",
               "SeQL intersection for cohort, API per-animal archive scan");
        db.clear_cache();
    }

    // Q-CROSS-2: Diet intervention activity score change — API FALLBACK
    {
        cout << "  Q-CROSS-2: Diet intervention BEHAV impact (API fallback)..." << endl;
        auto t0 = high_resolution_clock::now();
        int animals_checked = 0;
        double total_diff = 0.0;
        for (auto& aid : g_diet_intervention) {
            // Only if animal is also in BEHAV
            if (!db.is_element(aid, "BEHAV_Program")) continue;
            // Pre: weeks 14-17, Post: weeks 18-22
            double pre_sum = 0; int pre_n = 0;
            double post_sum = 0; int post_n = 0;
            for (int w = 14; w <= 17; w++) {
                char obs[64]; snprintf(obs, sizeof(obs), "%s_W%02d_B", aid.c_str(), w);
                int score = db.get_property_safe_int(obs, "activity_score");
                if (score > 0) { pre_sum += score; pre_n++; }
            }
            for (int w = 18; w <= 22; w++) {
                char obs[64]; snprintf(obs, sizeof(obs), "%s_W%02d_B", aid.c_str(), w);
                int score = db.get_property_safe_int(obs, "activity_score");
                if (score > 0) { post_sum += score; post_n++; }
            }
            if (pre_n > 0 && post_n > 0) {
                total_diff += (post_sum / post_n) - (pre_sum / pre_n);
                animals_checked++;
            }
        }
        double avg_diff = animals_checked > 0 ? total_diff / animals_checked : 0.0;
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        char note[128];
        snprintf(note, sizeof(note), "Avg activity score change: %.2f across %d animals", avg_diff, animals_checked);
        log_op("Q-CROSS", "API: Diet intervention BEHAV impact (W14-17 vs W18-22)",
               us, animals_checked, "API-Fallback", note);
    }

    // Q-CROSS-3: Temp deviation + vet treatment same week — API FALLBACK
    {
        cout << "  Q-CROSS-3: Temp deviation + vet treatment same week (API fallback)..." << endl;
        auto t0 = high_resolution_clock::now();
        set<string> matches;
        for (int w = 1; w <= 52; w++) {
            // Find deviated enclosures this week
            set<int> dev_encs;
            for (int enc = 1; enc <= 35; enc++) {
                char cond[48]; snprintf(cond, sizeof(cond), "ENC%02d_W%02d_Cond", enc, w);
                if (db.get_property_safe_bool(cond, "temp_deviation_flag"))
                    dev_encs.insert(enc);
            }
            if (dev_encs.empty()) continue;
            // Check VETSCI animals in deviated enclosures
            for (auto& aid : g_vetsci) {
                if (dev_encs.count(g_animal_enclosure[aid])) {
                    char vobs[64]; snprintf(vobs, sizeof(vobs), "%s_W%02d_V", aid.c_str(), w);
                    string tx = db.get_property_safe_string(vobs, "treatment_code");
                    if (!tx.empty() && tx != "NONE") matches.insert(aid);
                }
            }
        }
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        log_op("Q-CROSS", "API: Temp deviation + vet treatment same week (multi-hop)",
               us, (int)matches.size(), "API-Fallback",
               "enclosure condition -> enclosure -> animal -> VETSCI obs -> treatment");
    }

    // Q-CROSS-4: Species where >50% VETSCI animals had medical event — API FALLBACK
    {
        cout << "  Q-CROSS-4: Species with high medical event rate (API fallback)..." << endl;
        auto t0 = high_resolution_clock::now();
        // Group VETSCI animals by species
        map<string, vector<string>> species_animals;
        for (auto& aid : g_vetsci) {
            int si = g_animal_idx.count(aid) ? g_animals[g_animal_idx[aid]].species_idx : -1;
            if (si >= 0) species_animals[SPECIES[si].name].push_back(aid);
        }
        int flagged_species = 0;
        for (auto& [sp, animals] : species_animals) {
            int with_event = 0;
            for (auto& aid : animals) {
                for (int w = 1; w <= 52; w++) {
                    char obs[64]; snprintf(obs, sizeof(obs), "%s_W%02d_V", aid.c_str(), w);
                    if (db.get_property_safe_bool(obs, "medical_event")) {
                        with_event++;
                        break;
                    }
                }
            }
            if (with_event * 2 > (int)animals.size()) flagged_species++;
        }
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        log_op("Q-CROSS", "API: Species with >50% VETSCI medical event rate",
               us, flagged_species, "API-Fallback",
               "Aggregation and ratio computation not in SeQL");
    }

    // Q-CROSS-5: Animals with 4+ consecutive weight decline — API FALLBACK (canary query)
    {
        cout << "  Q-CROSS-5: Consecutive weight decline canary query (API fallback)..." << endl;
        auto t0 = high_resolution_clock::now();
        int flagged = 0;
        for (auto& aid : g_nutri) {
            vector<double> weights;
            for (int w = 1; w <= 52; w++) {
                char obs[64]; snprintf(obs, sizeof(obs), "%s_W%02d_N", aid.c_str(), w);
                double wt = db.get_property_safe_double(obs, "weight_kg");
                weights.push_back(wt);
            }
            // Check for 4+ consecutive declines
            int consec = 0;
            for (int i = 1; i < (int)weights.size(); i++) {
                if (weights[i] > 0 && weights[i - 1] > 0 && weights[i] < weights[i - 1])
                    consec++;
                else
                    consec = 0;
                if (consec >= 4) { flagged++; break; }
            }
        }
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        log_op("Q-CROSS", "API: Animals with 4+ consecutive weight decline (canary query)",
               us, flagged, "API-Fallback",
               "Sequential property comparison across ordered observations outside SeQL model");
    }
}

// =============================================================================
// STRESS CONDITIONS
// =============================================================================

void run_stress_conditions(cantordb& db, mt19937& rng) {
    cout << "\n=== STRESS CONDITIONS ===" << endl;

    // SC-1: Single week insert timing
    {
        cout << "  SC-1: Single week observation insert timing..." << endl;
        // Time was already captured during construction.
        // Re-run for one extra "week 53" as a clean benchmark
        auto t0 = high_resolution_clock::now();
        db.create_set("Week53_Observations");
        int ops = 0;
        for (auto& aid : g_behav) {
            string obs = aid + "_W53_B";
            db.create_set(obs);
            db.add_member("Week53_Observations", obs);
            db.add_member(aid + "_Archive", obs);
            db.add_member("BEHAV_Observations", obs);
            db.add_property("activity_score", 1 + (int)(rng() % 10), obs);
            db.add_property("stress_indicator", rand_stress(rng), obs);
            db.add_property("stereotypy_observed", (rng() % 10 == 0), obs);
            ops += 7;  // create + 3 memberships + 3 properties
        }
        for (auto& aid : g_nutri) {
            string obs = aid + "_W53_N";
            db.create_set(obs);
            db.add_member("Week53_Observations", obs);
            db.add_member(aid + "_Archive", obs);
            db.add_member("NUTRI_Observations", obs);
            db.add_property("weight_kg", 10.0 + (rng() % 500) / 10.0, obs);
            db.add_property("calories_consumed", 200 + (int)(rng() % 4000), obs);
            ops += 6;
        }
        for (auto& aid : g_enviro) {
            string obs = aid + "_W53_E";
            db.create_set(obs);
            db.add_member("Week53_Observations", obs);
            db.add_member(aid + "_Archive", obs);
            db.add_member("ENVIRO_Observations", obs);
            db.add_property("enrichment_provided", (rng() % 5 != 0), obs);
            db.add_property("noise_level", rand_noise(rng), obs);
            ops += 6;
        }
        auto t1 = high_resolution_clock::now();
        long long us = duration_cast<microseconds>(t1 - t0).count();
        long long per_op = ops > 0 ? us / ops : 0;
        char note[128];
        snprintf(note, sizeof(note), "%d total ops, %lld us/op avg", ops, per_op);
        log_op("SC", "SC-1: One week insert benchmark (Week53)", us, ops, "Pass", note);
        cout << "    " << ops << " ops in " << us << " us (" << per_op << " us/op)" << endl;

        // Clean up week 53 — trash it
        parse_query(db, "TRASH SETS Week53_Observations");
    }

    // SC-2: Large set cardinality
    {
        cout << "  SC-2: Large set cardinality..." << endl;
        string r1 = sq(db, "GET CARDINALITY OF BEHAV_Observations", "SC", "SC-2a: BEHAV_Observations cardinality");
        string r2 = sq(db, "GET CARDINALITY OF ENVIRO_Observations", "SC", "SC-2b: ENVIRO_Observations cardinality");
        string r3 = sq(db, "GET CARDINALITY OF UNIVERSAL", "SC", "SC-2c: UNIVERSAL cardinality");
        cout << "    BEHAV_Obs: " << r1 << "    ENVIRO_Obs: " << r2 << "    UNIVERSAL: " << r3;
    }

    // SC-3: WHERE filter over large set + cache check
    {
        cout << "  SC-3: WHERE filter over ENVIRO_Observations (2 runs)..." << endl;
        db.clear_cache();
        string q = "GET ELEMENTS OF ENVIRO_Observations WHERE enrichment_provided = false";
        auto t0 = high_resolution_clock::now();
        string r1 = parse_query(db, q);
        auto t1 = high_resolution_clock::now();
        long long us1 = duration_cast<microseconds>(t1 - t0).count();
        int c1 = count_lines(r1);
        log_op("SC", "SC-3a: " + q, us1, c1, "Pass", "First run");

        db.clear_cache();
        auto t2 = high_resolution_clock::now();
        string r2 = parse_query(db, q);
        auto t3 = high_resolution_clock::now();
        long long us2 = duration_cast<microseconds>(t3 - t2).count();
        int c2 = count_lines(r2);
        string cache_note = (us2 < us1) ? "Second run faster — possible caching" : "No implicit caching detected";
        log_op("SC", "SC-3b: " + q, us2, c2, "Pass",
               cache_note + " (run1: " + to_string(us1) + "us, run2: " + to_string(us2) + "us)");
        cout << "    Run 1: " << us1 << " us (" << c1 << " results), Run 2: " << us2 << " us (" << c2 << " results)" << endl;
        db.clear_cache();
    }

    // SC-4: Multi-term UNION parse limit
    {
        cout << "  SC-4: Multi-term UNION parse limit test..." << endl;
        int max_terms = 0;
        for (int n = 2; n <= 52; n++) {
            string query = "GET ELEMENTS OF Week01_Observations";
            for (int i = 2; i <= n; i++) {
                char buf[32]; snprintf(buf, sizeof(buf), " UNION Week%02d_Observations", i);
                query += buf;
            }
            auto t0 = high_resolution_clock::now();
            string r = parse_query(db, query);
            auto t1 = high_resolution_clock::now();
            long long us = duration_cast<microseconds>(t1 - t0).count();
            bool err = (r.find("Error") != string::npos) || (r.find("error") != string::npos);

            if (err) {
                log_op("SC", "SC-4: UNION chain FAILED at " + to_string(n) + " terms",
                       us, 0, "Fail", "Parser limit: " + to_string(n - 1) + " terms max");
                cout << "    Parser FAILED at " << n << " terms (max: " << n - 1 << ")" << endl;
                break;
            }
            max_terms = n;
            if (n == 52) {
                log_op("SC", "SC-4: UNION chain succeeded at 52 terms",
                       us, count_lines(r), "Pass", "Full 52-week union succeeded");
                cout << "    Full 52-term UNION succeeded (" << us << " us)" << endl;
            }
            db.clear_cache();
        }
        if (max_terms < 52) {
            cout << "    Maximum UNION chain length: " << max_terms << " terms" << endl;
        }
    }

    // SC-5: Dot notation under load
    {
        cout << "  SC-5: Dot notation precedence under load..." << endl;
        db.clear_cache();
        string q1 = "GET ELEMENTS OF BEHAV_Observations .INTERSECTION. VETSCI_Observations UNION NUTRI_Observations";
        string q2 = "GET ELEMENTS OF BEHAV_Observations INTERSECTION .VETSCI_Observations UNION. NUTRI_Observations";

        auto t0 = high_resolution_clock::now();
        string r1 = parse_query(db, q1);
        auto t1 = high_resolution_clock::now();
        long long us1 = duration_cast<microseconds>(t1 - t0).count();
        int c1 = count_lines(r1);
        bool err1 = r1.find("Error") != string::npos;
        log_op("SC", "SC-5a: " + q1, us1, c1, err1 ? "Fail" : "Pass",
               "INTERSECTION at priority 1");
        db.clear_cache();

        auto t2 = high_resolution_clock::now();
        string r2 = parse_query(db, q2);
        auto t3 = high_resolution_clock::now();
        long long us2 = duration_cast<microseconds>(t3 - t2).count();
        int c2 = count_lines(r2);
        bool err2 = r2.find("Error") != string::npos;
        log_op("SC", "SC-5b: " + q2, us2, c2, err2 ? "Fail" : "Pass",
               "UNION at priority 1");

        if (!err1 && !err2 && c1 != c2) {
            log_op("SC", "SC-5: Results differ as expected", 0, 0, "Pass",
                   "q1=" + to_string(c1) + " results, q2=" + to_string(c2) + " results — precedence working");
        } else if (!err1 && !err2 && c1 == c2) {
            log_op("SC", "SC-5: Results identical — possible precedence issue", 0, 0, "Pass",
                   "Both returned " + to_string(c1) + " results — may indicate flat evaluation");
        }
        cout << "    q1: " << c1 << " results (" << us1 << " us), q2: " << c2 << " results (" << us2 << " us)" << endl;
        db.clear_cache();
    }

    // SC-6: Archive intersection scaling
    {
        cout << "  SC-6: Archive intersection scaling..." << endl;
        // Use the first animal in AllPrograms_Cohort (enrolled in all 4)
        string aid = (g_animals.size() > 30) ? g_animals[30].id : g_animals[0].id;

        string q1 = "GET ELEMENTS OF " + aid + "_Archive INTERSECTION BEHAV_Observations";
        string q2 = "GET ELEMENTS OF " + aid + "_Archive INTERSECTION NUTRI_Observations";
        string q3 = "GET ELEMENTS OF " + aid + "_Archive INTERSECTION VETSCI_Observations";
        string q4 = "GET ELEMENTS OF " + aid + "_Archive INTERSECTION ENVIRO_Observations";

        string r1 = sq(db, q1, "SC", "SC-6a: " + aid + " archive x BEHAV");
        db.clear_cache();
        string r2 = sq(db, q2, "SC", "SC-6b: " + aid + " archive x NUTRI");
        db.clear_cache();
        string r3 = sq(db, q3, "SC", "SC-6c: " + aid + " archive x VETSCI");
        db.clear_cache();
        string r4 = sq(db, q4, "SC", "SC-6d: " + aid + " archive x ENVIRO");
        db.clear_cache();

        cout << "    " << aid << " archive intersections: BEHAV=" << count_lines(r1)
             << " NUTRI=" << count_lines(r2) << " VETSCI=" << count_lines(r3)
             << " ENVIRO=" << count_lines(r4) << endl;
    }

    // SC-7: TRASH and RESTORE observation container
    {
        cout << "  SC-7: TRASH/RESTORE of Week26_Observations..." << endl;

        // Pre-trash state
        string pre_card = parse_query(db, "GET CARDINALITY OF UNIVERSAL");
        string aid = g_behav.empty() ? g_animals[0].id : g_behav[0];

        sq(db, "TRASH SETS Week26_Observations", "SC", "SC-7a: Trash Week26_Observations");

        string post_card = sq(db, "GET CARDINALITY OF UNIVERSAL", "SC", "SC-7b: UNIVERSAL after trash");

        // Check if an individual W26 obs is still accessible via archive
        db.clear_cache();
        string archive_q = "GET ELEMENTS OF " + aid + "_Archive INTERSECTION BEHAV_Observations";
        string archive_r = sq(db, archive_q, "SC",
                              "SC-7c: Archive intersection after container trashed");

        // Does the result still contain W26 obs?
        bool w26_in_archive = (archive_r.find("_W26_") != string::npos);
        log_op("SC", "SC-7: W26 obs " + string(w26_in_archive ? "STILL" : "NOT") +
               " accessible via archive after container trash", 0, 0, "Pass",
               "Multi-membership soft-delete: container trashed, archive path " +
               string(w26_in_archive ? "still works" : "broken"));
        cout << "    W26 obs accessible via archive after container trash: "
             << (w26_in_archive ? "YES" : "NO") << endl;

        // Restore
        sq(db, "RESTORE Week26_Observations", "SC", "SC-7d: Restore Week26_Observations");
        string restored_card = sq(db, "GET CARDINALITY OF UNIVERSAL", "SC",
                                  "SC-7e: UNIVERSAL after restore");

        cout << "    UNIVERSAL cardinality: pre=" << pre_card
             << "  post-trash=" << post_card << "  post-restore=" << restored_card;
        db.clear_cache();
    }

    // SC-8: Full persistence fidelity
    {
        cout << "\n  SC-8: Full persistence fidelity test..." << endl;

        // Capture pre-save results for comparison
        cout << "    Capturing pre-save query results..." << endl;
        g_presave_results["BASE6_BEHAV"] = parse_query(db, "GET CARDINALITY OF BEHAV_Program");
        g_presave_results["BASE6_NUTRI"] = parse_query(db, "GET CARDINALITY OF NUTRI_Program");
        g_presave_results["BASE6_VETSCI"] = parse_query(db, "GET CARDINALITY OF VETSCI_Program");
        g_presave_results["BASE6_ENVIRO"] = parse_query(db, "GET CARDINALITY OF ENVIRO_Program");
        db.clear_cache();
        g_presave_results["BEHAV1"] = parse_query(db, "GET ELEMENTS OF BEHAV_Observations WHERE stress_indicator = High");
        db.clear_cache();
        g_presave_results["NUTRI1"] = parse_query(db, "GET ELEMENTS OF NUTRI_Observations WHERE diet_change_this_week = true");
        db.clear_cache();
        g_presave_results["VETSCI4"] = parse_query(db, "GET ELEMENTS OF MedEvents_Fatal");
        db.clear_cache();
        g_presave_results["ENVIRO1"] = parse_query(db, "GET ELEMENTS OF ENVIRO_Observations WHERE enrichment_provided = false");
        db.clear_cache();
        g_presave_results["SC2_BEHAV"] = parse_query(db, "GET CARDINALITY OF BEHAV_Observations");
        g_presave_results["SC2_ENVIRO"] = parse_query(db, "GET CARDINALITY OF ENVIRO_Observations");
        g_presave_results["SC2_UNIVERSAL"] = parse_query(db, "GET CARDINALITY OF UNIVERSAL");

        // Save
        cout << "    Saving full year database..." << endl;
        auto save_t0 = high_resolution_clock::now();
        string save_r = parse_query(db, "SAVE hzri_sc8_persistence");
        auto save_t1 = high_resolution_clock::now();
        long long save_us = duration_cast<microseconds>(save_t1 - save_t0).count();
        log_op("SC", "SC-8a: SAVE hzri_sc8_persistence", save_us, 0, "Pass",
               "Save time: " + to_string(save_us / 1000) + " ms");

        // Load into new instance
        cout << "    Loading database from file..." << endl;
        auto load_t0 = high_resolution_clock::now();
        cantordb* loaded = load_cantordb("hzri_sc8_persistence.cdb");
        auto load_t1 = high_resolution_clock::now();
        long long load_us = duration_cast<microseconds>(load_t1 - load_t0).count();

        if (!loaded) {
            log_op("SC", "SC-8: LOAD FAILED", load_us, 0, "Fail", "load_cantordb returned nullptr");
            cout << "    LOAD FAILED!" << endl;
            return;
        }
        log_op("SC", "SC-8b: LOAD hzri_sc8_persistence.cdb", load_us, 0, "Pass",
               "Load time: " + to_string(load_us / 1000) + " ms");
        cout << "    Load time: " << load_us / 1000 << " ms" << endl;

        // Re-run queries and compare
        cout << "    Verifying persistence fidelity..." << endl;
        int mismatches = 0;
        auto check = [&](const string& key, const string& query) {
            string result = parse_query(*loaded, query);
            if (result != g_presave_results[key]) {
                mismatches++;
                log_op("SC", "SC-8 MISMATCH: " + key, 0, 0, "Correctness-Failure",
                       "Pre: [" + g_presave_results[key].substr(0, 50) +
                       "] Post: [" + result.substr(0, 50) + "]");
            }
            loaded->clear_cache();
        };

        check("BASE6_BEHAV", "GET CARDINALITY OF BEHAV_Program");
        check("BASE6_NUTRI", "GET CARDINALITY OF NUTRI_Program");
        check("BASE6_VETSCI", "GET CARDINALITY OF VETSCI_Program");
        check("BASE6_ENVIRO", "GET CARDINALITY OF ENVIRO_Program");
        check("BEHAV1", "GET ELEMENTS OF BEHAV_Observations WHERE stress_indicator = High");
        loaded->clear_cache();
        check("NUTRI1", "GET ELEMENTS OF NUTRI_Observations WHERE diet_change_this_week = true");
        loaded->clear_cache();
        check("VETSCI4", "GET ELEMENTS OF MedEvents_Fatal");
        check("ENVIRO1", "GET ELEMENTS OF ENVIRO_Observations WHERE enrichment_provided = false");
        loaded->clear_cache();
        check("SC2_BEHAV", "GET CARDINALITY OF BEHAV_Observations");
        check("SC2_ENVIRO", "GET CARDINALITY OF ENVIRO_Observations");
        check("SC2_UNIVERSAL", "GET CARDINALITY OF UNIVERSAL");

        string status = (mismatches == 0) ? "Pass" : "Correctness-Failure";
        log_op("SC", "SC-8: Persistence fidelity check", 0, mismatches, status,
               to_string(mismatches) + " mismatches out of 11 checks");
        cout << "    Persistence fidelity: " << (mismatches == 0 ? "PERFECT" : to_string(mismatches) + " MISMATCHES") << endl;

        delete loaded;
    }
}

// =============================================================================
// FINAL REPORT
// =============================================================================

void write_report() {
    cout << "\n" << string(72, '=') << endl;
    cout << "HZRI STRESS TEST — FINAL REPORT" << endl;
    cout << string(72, '=') << endl;

    // 1. Total operation counts by status
    cout << "\n1. Operation Counts by Status:" << endl;
    cout << "   Pass:                " << g_pass << endl;
    cout << "   Fail:                " << g_fail << endl;
    cout << "   Correctness-Failure: " << g_correctness_fail << endl;
    cout << "   API-Fallback:        " << g_api_fallback << endl;
    cout << "   Partial-Hybrid:      " << g_partial_hybrid << endl;
    cout << "   Total logged:        " << g_log.size() << endl;

    // 2. API-Fallback gap analysis
    cout << "\n2. API-Fallback Gap Analysis:" << endl;
    map<string, vector<string>> fallback_reasons;
    for (auto& op : g_log) {
        if (op.status == "API-Fallback" || op.status == "Partial-Hybrid") {
            string reason = op.notes;
            // Categorize by reason type
            string category;
            if (reason.find("traversal") != string::npos || reason.find("multi-hop") != string::npos ||
                reason.find("membership") != string::npos || reason.find("inverted") != string::npos)
                category = "Multi-level membership traversal";
            else if (reason.find("ORDER") != string::npos || reason.find("order") != string::npos ||
                     reason.find("temporal") != string::npos || reason.find("sequential") != string::npos)
                category = "Temporal ordering";
            else if (reason.find("aggregation") != string::npos || reason.find("arithmetic") != string::npos ||
                     reason.find("ratio") != string::npos || reason.find("average") != string::npos)
                category = "Aggregation/arithmetic";
            else if (reason.find("iterate") != string::npos || reason.find("cohort") != string::npos ||
                     reason.find("per-animal") != string::npos || reason.find("bulk") != string::npos)
                category = "Per-cohort bulk iteration";
            else
                category = "Other";
            fallback_reasons[category].push_back(op.op_id + ": " + op.seql_or_api.substr(0, 80));
        }
    }
    for (auto& [reason, ops] : fallback_reasons) {
        cout << "   " << reason << " (" << ops.size() << " operations):" << endl;
        for (auto& op : ops) {
            cout << "     - " << op << endl;
        }
    }

    // 3. Slowest 10 operations
    cout << "\n3. Slowest 10 Operations:" << endl;
    vector<size_t> indices(g_log.size());
    iota(indices.begin(), indices.end(), 0);
    partial_sort(indices.begin(), indices.begin() + min((size_t)10, indices.size()),
                 indices.end(), [](size_t a, size_t b) { return g_log[a].duration_us > g_log[b].duration_us; });
    for (int i = 0; i < min(10, (int)g_log.size()); i++) {
        auto& op = g_log[indices[i]];
        cout << "   " << setw(3) << (i + 1) << ". " << setw(12) << op.duration_us << " us | "
             << setw(20) << op.category << " | " << op.seql_or_api.substr(0, 60) << endl;
    }

    // 4-6. Key results summary
    cout << "\n4. SC-4 Result: Maximum UNION chain length — see SC-4 entries in log" << endl;
    cout << "5. SC-7 Result: Soft-delete semantics — see SC-7 entries in log" << endl;
    cout << "6. SC-8 Result: UNIVERSAL cardinality + load time — see SC-8 entries in log" << endl;

    // For convenience, find and print SC results
    for (auto& op : g_log) {
        if (op.op_id.find("SC") == 0) {
            if (op.notes.find("Parser limit") != string::npos ||
                op.notes.find("52-week union") != string::npos ||
                op.notes.find("Multi-membership") != string::npos ||
                op.notes.find("Load time") != string::npos ||
                op.notes.find("Persistence fidelity") != string::npos) {
                cout << "   [" << op.op_id << "] " << op.notes << endl;
            }
        }
    }

    // Write CSV log
    ofstream csv("hzri_stress_log.csv");
    if (csv.is_open()) {
        csv << "op_id,category,seql_or_api,duration_us,result_count,status,notes" << endl;
        for (auto& op : g_log) {
            // Escape commas in fields
            auto esc = [](const string& s) -> string {
                if (s.find(',') != string::npos || s.find('"') != string::npos) {
                    string e = s;
                    size_t pos = 0;
                    while ((pos = e.find('"', pos)) != string::npos) {
                        e.insert(pos, "\"");
                        pos += 2;
                    }
                    return "\"" + e + "\"";
                }
                return s;
            };
            csv << esc(op.op_id) << "," << esc(op.category) << ","
                << esc(op.seql_or_api) << "," << op.duration_us << ","
                << op.result_count << "," << esc(op.status) << ","
                << esc(op.notes) << endl;
        }
        csv.close();
        cout << "\nFull log written to hzri_stress_log.csv" << endl;
    }

    cout << "\n" << string(72, '=') << endl;
    cout << (g_fail + g_correctness_fail > 0 ? "SOME FAILURES DETECTED" : "ALL CHECKS PASSED") << endl;
    cout << string(72, '=') << endl;
}

// =============================================================================
// MAIN
// =============================================================================

int main() {
    cout << string(72, '=') << endl;
    cout << "HZRI STRESS TEST — Hartwell Zoological Research Institute" << endl;
    cout << "CantorDB v0.1.0 Research Edition Stress Test" << endl;
    cout << string(72, '=') << endl;

    auto global_t0 = high_resolution_clock::now();

    mt19937 rng(42);  // deterministic seed

    // Generate animal data
    generate_animals(rng);

    // Create database
    cantordb db("hzri");

    // Phase 1: Taxonomy
    build_taxonomy(db);

    // Phase 2: Animals & Enclosures
    build_animals(db);

    // Phase 3: Research Enrollment
    build_enrollment(db);

    // Phases 4-7: Weekly observations, conditions, events
    build_weekly_data(db, rng);

    // Clear cache before queries
    db.clear_cache();

    // === QUERIES ===
    run_base_queries(db);
    db.clear_cache();

    run_behav_queries(db);
    db.clear_cache();

    run_nutri_queries(db);
    db.clear_cache();

    run_vetsci_queries(db);
    db.clear_cache();

    run_enviro_queries(db);
    db.clear_cache();

    run_cross_queries(db);
    db.clear_cache();

    // === STRESS CONDITIONS ===
    run_stress_conditions(db, rng);

    auto global_t1 = high_resolution_clock::now();
    long long total_ms = duration_cast<milliseconds>(global_t1 - global_t0).count();
    cout << "\nTotal test time: " << total_ms / 1000 << "." << (total_ms % 1000) / 100 << " seconds" << endl;

    // === REPORT ===
    write_report();

    return (g_fail + g_correctness_fail > 0) ? 1 : 0;
}
