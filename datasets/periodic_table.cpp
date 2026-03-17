#include "cantordb.h"
#include <iostream>
#include <vector>

using namespace std;

struct Element {
	int atomic_number;
	string name;
	string symbol;
	double atomic_weight;
	int period;
	int group;          // 0 for lanthanides/actinides
	string category;    // AlkaliMetals, AlkalineEarthMetals, TransitionMetals,
	                    // PostTransitionMetals, Lanthanides, Actinides,
	                    // Metalloids, ReactiveNonmetals, NobleGases
	string state;       // Solid, Liquid, Gas
	bool radioactive;
	bool synthetic;
};

static vector<Element> elements = {
	// Period 1
	{1,   "Hydrogen",      "H",   1.008,    1, 1,  "ReactiveNonmetals", "Gas",    false, false},
	{2,   "Helium",        "He",  4.0026,   1, 18, "NobleGases",        "Gas",    false, false},
	// Period 2
	{3,   "Lithium",       "Li",  6.941,    2, 1,  "AlkaliMetals",      "Solid",  false, false},
	{4,   "Beryllium",     "Be",  9.0122,   2, 2,  "AlkalineEarthMetals","Solid", false, false},
	{5,   "Boron",         "B",   10.81,    2, 13, "Metalloids",        "Solid",  false, false},
	{6,   "Carbon",        "C",   12.011,   2, 14, "ReactiveNonmetals", "Solid",  false, false},
	{7,   "Nitrogen",      "N",   14.007,   2, 15, "ReactiveNonmetals", "Gas",    false, false},
	{8,   "Oxygen",        "O",   15.999,   2, 16, "ReactiveNonmetals", "Gas",    false, false},
	{9,   "Fluorine",      "F",   18.998,   2, 17, "ReactiveNonmetals", "Gas",    false, false},
	{10,  "Neon",          "Ne",  20.180,   2, 18, "NobleGases",        "Gas",    false, false},
	// Period 3
	{11,  "Sodium",        "Na",  22.990,   3, 1,  "AlkaliMetals",      "Solid",  false, false},
	{12,  "Magnesium",     "Mg",  24.305,   3, 2,  "AlkalineEarthMetals","Solid", false, false},
	{13,  "Aluminium",     "Al",  26.982,   3, 13, "PostTransitionMetals","Solid",false, false},
	{14,  "Silicon",       "Si",  28.085,   3, 14, "Metalloids",        "Solid",  false, false},
	{15,  "Phosphorus",    "P",   30.974,   3, 15, "ReactiveNonmetals", "Solid",  false, false},
	{16,  "Sulfur",        "S",   32.06,    3, 16, "ReactiveNonmetals", "Solid",  false, false},
	{17,  "Chlorine",      "Cl",  35.45,    3, 17, "ReactiveNonmetals", "Gas",    false, false},
	{18,  "Argon",         "Ar",  39.948,   3, 18, "NobleGases",        "Gas",    false, false},
	// Period 4
	{19,  "Potassium",     "K",   39.098,   4, 1,  "AlkaliMetals",      "Solid",  false, false},
	{20,  "Calcium",       "Ca",  40.078,   4, 2,  "AlkalineEarthMetals","Solid", false, false},
	{21,  "Scandium",      "Sc",  44.956,   4, 3,  "TransitionMetals",  "Solid",  false, false},
	{22,  "Titanium",      "Ti",  47.867,   4, 4,  "TransitionMetals",  "Solid",  false, false},
	{23,  "Vanadium",      "V",   50.942,   4, 5,  "TransitionMetals",  "Solid",  false, false},
	{24,  "Chromium",      "Cr",  51.996,   4, 6,  "TransitionMetals",  "Solid",  false, false},
	{25,  "Manganese",     "Mn",  54.938,   4, 7,  "TransitionMetals",  "Solid",  false, false},
	{26,  "Iron",          "Fe",  55.845,   4, 8,  "TransitionMetals",  "Solid",  false, false},
	{27,  "Cobalt",        "Co",  58.933,   4, 9,  "TransitionMetals",  "Solid",  false, false},
	{28,  "Nickel",        "Ni",  58.693,   4, 10, "TransitionMetals",  "Solid",  false, false},
	{29,  "Copper",        "Cu",  63.546,   4, 11, "TransitionMetals",  "Solid",  false, false},
	{30,  "Zinc",          "Zn",  65.38,    4, 12, "TransitionMetals",  "Solid",  false, false},
	{31,  "Gallium",       "Ga",  69.723,   4, 13, "PostTransitionMetals","Solid",false, false},
	{32,  "Germanium",     "Ge",  72.630,   4, 14, "Metalloids",        "Solid",  false, false},
	{33,  "Arsenic",       "As",  74.922,   4, 15, "Metalloids",        "Solid",  false, false},
	{34,  "Selenium",      "Se",  78.971,   4, 16, "ReactiveNonmetals", "Solid",  false, false},
	{35,  "Bromine",       "Br",  79.904,   4, 17, "ReactiveNonmetals", "Liquid", false, false},
	{36,  "Krypton",       "Kr",  83.798,   4, 18, "NobleGases",        "Gas",    false, false},
	// Period 5
	{37,  "Rubidium",      "Rb",  85.468,   5, 1,  "AlkaliMetals",      "Solid",  false, false},
	{38,  "Strontium",     "Sr",  87.62,    5, 2,  "AlkalineEarthMetals","Solid", false, false},
	{39,  "Yttrium",       "Y",   88.906,   5, 3,  "TransitionMetals",  "Solid",  false, false},
	{40,  "Zirconium",     "Zr",  91.224,   5, 4,  "TransitionMetals",  "Solid",  false, false},
	{41,  "Niobium",       "Nb",  92.906,   5, 5,  "TransitionMetals",  "Solid",  false, false},
	{42,  "Molybdenum",    "Mo",  95.95,    5, 6,  "TransitionMetals",  "Solid",  false, false},
	{43,  "Technetium",    "Tc",  98.0,     5, 7,  "TransitionMetals",  "Solid",  true,  true},
	{44,  "Ruthenium",     "Ru",  101.07,   5, 8,  "TransitionMetals",  "Solid",  false, false},
	{45,  "Rhodium",       "Rh",  102.91,   5, 9,  "TransitionMetals",  "Solid",  false, false},
	{46,  "Palladium",     "Pd",  106.42,   5, 10, "TransitionMetals",  "Solid",  false, false},
	{47,  "Silver",        "Ag",  107.87,   5, 11, "TransitionMetals",  "Solid",  false, false},
	{48,  "Cadmium",       "Cd",  112.41,   5, 12, "TransitionMetals",  "Solid",  false, false},
	{49,  "Indium",        "In",  114.82,   5, 13, "PostTransitionMetals","Solid",false, false},
	{50,  "Tin",           "Sn",  118.71,   5, 14, "PostTransitionMetals","Solid",false, false},
	{51,  "Antimony",      "Sb",  121.76,   5, 15, "Metalloids",        "Solid",  false, false},
	{52,  "Tellurium",     "Te",  127.60,   5, 16, "Metalloids",        "Solid",  false, false},
	{53,  "Iodine",        "I",   126.90,   5, 17, "ReactiveNonmetals", "Solid",  false, false},
	{54,  "Xenon",         "Xe",  131.29,   5, 18, "NobleGases",        "Gas",    false, false},
	// Period 6
	{55,  "Caesium",       "Cs",  132.91,   6, 1,  "AlkaliMetals",      "Solid",  false, false},
	{56,  "Barium",        "Ba",  137.33,   6, 2,  "AlkalineEarthMetals","Solid", false, false},
	// Lanthanides (group 0)
	{57,  "Lanthanum",     "La",  138.91,   6, 0,  "Lanthanides",       "Solid",  false, false},
	{58,  "Cerium",        "Ce",  140.12,   6, 0,  "Lanthanides",       "Solid",  false, false},
	{59,  "Praseodymium",  "Pr",  140.91,   6, 0,  "Lanthanides",       "Solid",  false, false},
	{60,  "Neodymium",     "Nd",  144.24,   6, 0,  "Lanthanides",       "Solid",  false, false},
	{61,  "Promethium",    "Pm",  145.0,    6, 0,  "Lanthanides",       "Solid",  true,  true},
	{62,  "Samarium",      "Sm",  150.36,   6, 0,  "Lanthanides",       "Solid",  false, false},
	{63,  "Europium",      "Eu",  151.96,   6, 0,  "Lanthanides",       "Solid",  false, false},
	{64,  "Gadolinium",    "Gd",  157.25,   6, 0,  "Lanthanides",       "Solid",  false, false},
	{65,  "Terbium",       "Tb",  158.93,   6, 0,  "Lanthanides",       "Solid",  false, false},
	{66,  "Dysprosium",    "Dy",  162.50,   6, 0,  "Lanthanides",       "Solid",  false, false},
	{67,  "Holmium",       "Ho",  164.93,   6, 0,  "Lanthanides",       "Solid",  false, false},
	{68,  "Erbium",        "Er",  167.26,   6, 0,  "Lanthanides",       "Solid",  false, false},
	{69,  "Thulium",       "Tm",  168.93,   6, 0,  "Lanthanides",       "Solid",  false, false},
	{70,  "Ytterbium",     "Yb",  173.05,   6, 0,  "Lanthanides",       "Solid",  false, false},
	{71,  "Lutetium",      "Lu",  174.97,   6, 0,  "Lanthanides",       "Solid",  false, false},
	// Back to period 6 main
	{72,  "Hafnium",       "Hf",  178.49,   6, 4,  "TransitionMetals",  "Solid",  false, false},
	{73,  "Tantalum",      "Ta",  180.95,   6, 5,  "TransitionMetals",  "Solid",  false, false},
	{74,  "Tungsten",      "W",   183.84,   6, 6,  "TransitionMetals",  "Solid",  false, false},
	{75,  "Rhenium",       "Re",  186.21,   6, 7,  "TransitionMetals",  "Solid",  false, false},
	{76,  "Osmium",        "Os",  190.23,   6, 8,  "TransitionMetals",  "Solid",  false, false},
	{77,  "Iridium",       "Ir",  192.22,   6, 9,  "TransitionMetals",  "Solid",  false, false},
	{78,  "Platinum",      "Pt",  195.08,   6, 10, "TransitionMetals",  "Solid",  false, false},
	{79,  "Gold",          "Au",  196.97,   6, 11, "TransitionMetals",  "Solid",  false, false},
	{80,  "Mercury",       "Hg",  200.59,   6, 12, "TransitionMetals",  "Liquid", false, false},
	{81,  "Thallium",      "Tl",  204.38,   6, 13, "PostTransitionMetals","Solid",false, false},
	{82,  "Lead",          "Pb",  207.2,    6, 14, "PostTransitionMetals","Solid",false, false},
	{83,  "Bismuth",       "Bi",  208.98,   6, 15, "PostTransitionMetals","Solid",true,  false},
	{84,  "Polonium",      "Po",  209.0,    6, 16, "Metalloids",        "Solid",  true,  false},
	{85,  "Astatine",      "At",  210.0,    6, 17, "Metalloids",        "Solid",  true,  false},
	{86,  "Radon",         "Rn",  222.0,    6, 18, "NobleGases",        "Gas",    true,  false},
	// Period 7
	{87,  "Francium",      "Fr",  223.0,    7, 1,  "AlkaliMetals",      "Solid",  true,  false},
	{88,  "Radium",        "Ra",  226.0,    7, 2,  "AlkalineEarthMetals","Solid", true,  false},
	// Actinides (group 0)
	{89,  "Actinium",      "Ac",  227.0,    7, 0,  "Actinides",         "Solid",  true,  false},
	{90,  "Thorium",       "Th",  232.04,   7, 0,  "Actinides",         "Solid",  true,  false},
	{91,  "Protactinium",  "Pa",  231.04,   7, 0,  "Actinides",         "Solid",  true,  false},
	{92,  "Uranium",       "U",   238.03,   7, 0,  "Actinides",         "Solid",  true,  false},
	{93,  "Neptunium",     "Np",  237.0,    7, 0,  "Actinides",         "Solid",  true,  true},
	{94,  "Plutonium",     "Pu",  244.0,    7, 0,  "Actinides",         "Solid",  true,  true},
	{95,  "Americium",     "Am",  243.0,    7, 0,  "Actinides",         "Solid",  true,  true},
	{96,  "Curium",        "Cm",  247.0,    7, 0,  "Actinides",         "Solid",  true,  true},
	{97,  "Berkelium",     "Bk",  247.0,    7, 0,  "Actinides",         "Solid",  true,  true},
	{98,  "Californium",   "Cf",  251.0,    7, 0,  "Actinides",         "Solid",  true,  true},
	{99,  "Einsteinium",   "Es",  252.0,    7, 0,  "Actinides",         "Solid",  true,  true},
	{100, "Fermium",       "Fm",  257.0,    7, 0,  "Actinides",         "Solid",  true,  true},
	{101, "Mendelevium",   "Md",  258.0,    7, 0,  "Actinides",         "Solid",  true,  true},
	{102, "Nobelium",      "No",  259.0,    7, 0,  "Actinides",         "Solid",  true,  true},
	{103, "Lawrencium",    "Lr",  266.0,    7, 0,  "Actinides",         "Solid",  true,  true},
	// Back to period 7 main
	{104, "Rutherfordium", "Rf",  267.0,    7, 4,  "TransitionMetals",  "Solid",  true,  true},
	{105, "Dubnium",       "Db",  268.0,    7, 5,  "TransitionMetals",  "Solid",  true,  true},
	{106, "Seaborgium",    "Sg",  269.0,    7, 6,  "TransitionMetals",  "Solid",  true,  true},
	{107, "Bohrium",       "Bh",  270.0,    7, 7,  "TransitionMetals",  "Solid",  true,  true},
	{108, "Hassium",       "Hs",  277.0,    7, 8,  "TransitionMetals",  "Solid",  true,  true},
	{109, "Meitnerium",    "Mt",  278.0,    7, 9,  "TransitionMetals",  "Solid",  true,  true},
	{110, "Darmstadtium",  "Ds",  281.0,    7, 10, "TransitionMetals",  "Solid",  true,  true},
	{111, "Roentgenium",   "Rg",  282.0,    7, 11, "TransitionMetals",  "Solid",  true,  true},
	{112, "Copernicium",   "Cn",  285.0,    7, 12, "TransitionMetals",  "Solid",  true,  true},
	{113, "Nihonium",      "Nh",  286.0,    7, 13, "PostTransitionMetals","Solid",true,  true},
	{114, "Flerovium",     "Fl",  289.0,    7, 14, "PostTransitionMetals","Solid",true,  true},
	{115, "Moscovium",     "Mc",  290.0,    7, 15, "PostTransitionMetals","Solid",true,  true},
	{116, "Livermorium",   "Lv",  293.0,    7, 16, "PostTransitionMetals","Solid",true,  true},
	{117, "Tennessine",    "Ts",  294.0,    7, 17, "ReactiveNonmetals", "Solid",  true,  true},
	{118, "Oganesson",     "Og",  294.0,    7, 18, "NobleGases",        "Gas",    true,  true},
};

int main() {
	cantordb db("PeriodicTable");

	// --- Register property types ---
	db.create_property("atomic_number", "int");
	db.create_property("symbol", "string");
	db.create_property("atomic_weight", "double");
	db.create_property("period", "int");
	db.create_property("group", "int");
	db.create_property("description", "string");
	db.create_property("typical_charge", "int");

	// --- Classification category sets ---
	// Metal sub-categories
	db.create_set("Metals");
	db.create_set("AlkaliMetals");
	db.create_set("AlkalineEarthMetals");
	db.create_set("TransitionMetals");
	db.create_set("PostTransitionMetals");
	db.create_set("Lanthanides");
	db.create_set("Actinides");

	// Metal hierarchy
	db.add_member("Metals", "AlkaliMetals");
	db.add_member("Metals", "AlkalineEarthMetals");
	db.add_member("Metals", "TransitionMetals");
	db.add_member("Metals", "PostTransitionMetals");
	db.add_member("Metals", "Lanthanides");
	db.add_member("Metals", "Actinides");

	// Nonmetal sub-categories
	db.create_set("Nonmetals");
	db.create_set("ReactiveNonmetals");
	db.create_set("NobleGases");

	db.add_member("Nonmetals", "ReactiveNonmetals");
	db.add_member("Nonmetals", "NobleGases");

	// Metalloids (standalone)
	db.create_set("Metalloids");

	// State at room temperature
	db.create_set("Solid");
	db.create_set("Liquid");
	db.create_set("Gas");

	// Stability
	db.create_set("Radioactive");
	db.create_set("Stable");

	// Origin
	db.create_set("Natural");
	db.create_set("Synthetic");

	// Period sets
	db.create_set("Period1");
	db.create_set("Period2");
	db.create_set("Period3");
	db.create_set("Period4");
	db.create_set("Period5");
	db.create_set("Period6");
	db.create_set("Period7");

	// Group sets (1-18)
	for (int g = 1; g <= 18; g++) {
		db.create_set("Group" + to_string(g));
	}

	// Magnetic elements (ferromagnetic at room temperature)
	db.create_set("Ferromagnetic");
	db.add_property("description", string("Elements that are ferromagnetic at room temperature"), "Ferromagnetic");

	// Paramagnetic (attracted to magnetic fields but not permanently magnetized)
	db.create_set("Paramagnetic");
	db.add_property("description", string("Elements attracted weakly to magnetic fields"), "Paramagnetic");

	// Diamagnetic (repelled by magnetic fields)
	db.create_set("Diamagnetic");
	db.add_property("description", string("Elements weakly repelled by magnetic fields"), "Diamagnetic");

	// Diatomic elements (exist as two-atom molecules in standard conditions)
	db.create_set("Diatomic");
	db.add_property("description", string("Elements that naturally form two-atom molecules"), "Diatomic");

	// --- Create each element and classify ---
	for (auto& e : elements) {
		db.create_set(e.name);

		// Properties
		db.add_property("atomic_number", e.atomic_number, e.name);
		db.add_property("symbol", e.symbol, e.name);
		db.add_property("atomic_weight", e.atomic_weight, e.name);
		db.add_property("period", e.period, e.name);
		db.add_property("group", e.group, e.name);

		// Add to chemical category (and parent)
		db.add_member(e.category, e.name);
		if (e.category == "AlkaliMetals" || e.category == "AlkalineEarthMetals" ||
		    e.category == "TransitionMetals" || e.category == "PostTransitionMetals" ||
		    e.category == "Lanthanides" || e.category == "Actinides") {
			db.add_member("Metals", e.name);
		} else if (e.category == "ReactiveNonmetals" || e.category == "NobleGases") {
			db.add_member("Nonmetals", e.name);
		}

		// State
		db.add_member(e.state, e.name);

		// Radioactivity
		db.add_member(e.radioactive ? "Radioactive" : "Stable", e.name);

		// Origin
		db.add_member(e.synthetic ? "Synthetic" : "Natural", e.name);

		// Period
		db.add_member("Period" + to_string(e.period), e.name);

		// Group (0 means lanthanide/actinide, no standard group)
		if (e.group > 0) {
			db.add_member("Group" + to_string(e.group), e.name);
		}
	}

	// --- Diatomic elements (H2, N2, O2, F2, Cl2, Br2, I2) ---
	db.add_member("Diatomic", "Hydrogen");
	db.add_member("Diatomic", "Nitrogen");
	db.add_member("Diatomic", "Oxygen");
	db.add_member("Diatomic", "Fluorine");
	db.add_member("Diatomic", "Chlorine");
	db.add_member("Diatomic", "Bromine");
	db.add_member("Diatomic", "Iodine");

	// --- Ferromagnetic elements (at room temperature) ---
	db.add_member("Ferromagnetic", "Iron");
	db.add_member("Ferromagnetic", "Cobalt");
	db.add_member("Ferromagnetic", "Nickel");
	db.add_member("Ferromagnetic", "Gadolinium");

	// --- Paramagnetic elements ---
	// Alkali metals
	db.add_member("Paramagnetic", "Lithium");
	db.add_member("Paramagnetic", "Sodium");
	db.add_member("Paramagnetic", "Potassium");
	db.add_member("Paramagnetic", "Rubidium");
	db.add_member("Paramagnetic", "Caesium");
	// Alkaline earth
	db.add_member("Paramagnetic", "Magnesium");
	db.add_member("Paramagnetic", "Calcium");
	db.add_member("Paramagnetic", "Strontium");
	db.add_member("Paramagnetic", "Barium");
	// Transition metals
	db.add_member("Paramagnetic", "Scandium");
	db.add_member("Paramagnetic", "Titanium");
	db.add_member("Paramagnetic", "Vanadium");
	db.add_member("Paramagnetic", "Chromium");
	db.add_member("Paramagnetic", "Manganese");
	db.add_member("Paramagnetic", "Molybdenum");
	db.add_member("Paramagnetic", "Ruthenium");
	db.add_member("Paramagnetic", "Rhodium");
	db.add_member("Paramagnetic", "Palladium");
	db.add_member("Paramagnetic", "Tungsten");
	db.add_member("Paramagnetic", "Platinum");
	// Lanthanides (most are paramagnetic)
	db.add_member("Paramagnetic", "Cerium");
	db.add_member("Paramagnetic", "Praseodymium");
	db.add_member("Paramagnetic", "Neodymium");
	db.add_member("Paramagnetic", "Samarium");
	db.add_member("Paramagnetic", "Europium");
	db.add_member("Paramagnetic", "Terbium");
	db.add_member("Paramagnetic", "Dysprosium");
	db.add_member("Paramagnetic", "Holmium");
	db.add_member("Paramagnetic", "Erbium");
	db.add_member("Paramagnetic", "Thulium");
	db.add_member("Paramagnetic", "Ytterbium");
	// Actinides
	db.add_member("Paramagnetic", "Uranium");
	db.add_member("Paramagnetic", "Plutonium");
	db.add_member("Paramagnetic", "Curium");
	// Other
	db.add_member("Paramagnetic", "Aluminium");
	db.add_member("Paramagnetic", "Oxygen");

	// --- Diamagnetic elements ---
	db.add_member("Diamagnetic", "Hydrogen");
	db.add_member("Diamagnetic", "Helium");
	db.add_member("Diamagnetic", "Beryllium");
	db.add_member("Diamagnetic", "Boron");
	db.add_member("Diamagnetic", "Carbon");
	db.add_member("Diamagnetic", "Nitrogen");
	db.add_member("Diamagnetic", "Fluorine");
	db.add_member("Diamagnetic", "Neon");
	db.add_member("Diamagnetic", "Silicon");
	db.add_member("Diamagnetic", "Phosphorus");
	db.add_member("Diamagnetic", "Sulfur");
	db.add_member("Diamagnetic", "Chlorine");
	db.add_member("Diamagnetic", "Argon");
	db.add_member("Diamagnetic", "Copper");
	db.add_member("Diamagnetic", "Zinc");
	db.add_member("Diamagnetic", "Gallium");
	db.add_member("Diamagnetic", "Germanium");
	db.add_member("Diamagnetic", "Arsenic");
	db.add_member("Diamagnetic", "Selenium");
	db.add_member("Diamagnetic", "Bromine");
	db.add_member("Diamagnetic", "Krypton");
	db.add_member("Diamagnetic", "Silver");
	db.add_member("Diamagnetic", "Cadmium");
	db.add_member("Diamagnetic", "Indium");
	db.add_member("Diamagnetic", "Tin");
	db.add_member("Diamagnetic", "Antimony");
	db.add_member("Diamagnetic", "Tellurium");
	db.add_member("Diamagnetic", "Iodine");
	db.add_member("Diamagnetic", "Xenon");
	db.add_member("Diamagnetic", "Gold");
	db.add_member("Diamagnetic", "Mercury");
	db.add_member("Diamagnetic", "Thallium");
	db.add_member("Diamagnetic", "Lead");
	db.add_member("Diamagnetic", "Bismuth");
	db.add_member("Diamagnetic", "Lutetium");

	// --- Add shared properties to category sets ---
	db.add_property("description", string("Soft, shiny, highly reactive metals"), "AlkaliMetals");
	db.add_property("typical_charge", 1, "AlkaliMetals");

	db.add_property("description", string("Shiny, silvery-white, reactive metals"), "AlkalineEarthMetals");
	db.add_property("typical_charge", 2, "AlkalineEarthMetals");

	db.add_property("description", string("Hard metals with high melting points and variable oxidation states"), "TransitionMetals");

	db.add_property("description", string("Metals with lower melting points, softer than transition metals"), "PostTransitionMetals");

	db.add_property("description", string("Rare earth metals with similar chemical properties"), "Lanthanides");
	db.add_property("typical_charge", 3, "Lanthanides");

	db.add_property("description", string("Radioactive metals, most are synthetic"), "Actinides");

	db.add_property("description", string("Properties intermediate between metals and nonmetals"), "Metalloids");

	db.add_property("description", string("Chemically active nonmetals"), "ReactiveNonmetals");

	db.add_property("description", string("Extremely low reactivity, colorless, odorless gases"), "NobleGases");
	db.add_property("typical_charge", 0, "NobleGases");

	// Save
	string path = "periodic_table.cdb";
	if (save_cantordb(db, path)) {
		cout << "Saved periodic table database to " << path << endl;
	} else {
		cout << "Error saving database." << endl;
		return 1;
	}

	// --- Verification ---
	cout << endl << "=== Dataset Summary ===" << endl;
	cout << "Total elements: " << db.get_cardinality(UNIVERSAL_SET) << endl;
	cout << "Metals: " << db.get_cardinality("Metals") << endl;
	cout << "  Alkali Metals: " << db.get_cardinality("AlkaliMetals") << endl;
	cout << "  Alkaline Earth Metals: " << db.get_cardinality("AlkalineEarthMetals") << endl;
	cout << "  Transition Metals: " << db.get_cardinality("TransitionMetals") << endl;
	cout << "  Post-Transition Metals: " << db.get_cardinality("PostTransitionMetals") << endl;
	cout << "  Lanthanides: " << db.get_cardinality("Lanthanides") << endl;
	cout << "  Actinides: " << db.get_cardinality("Actinides") << endl;
	cout << "Metalloids: " << db.get_cardinality("Metalloids") << endl;
	cout << "Nonmetals: " << db.get_cardinality("Nonmetals") << endl;
	cout << "  Reactive Nonmetals: " << db.get_cardinality("ReactiveNonmetals") << endl;
	cout << "  Noble Gases: " << db.get_cardinality("NobleGases") << endl;
	cout << endl;
	cout << "Solid: " << db.get_cardinality("Solid") << endl;
	cout << "Liquid: " << db.get_cardinality("Liquid") << endl;
	cout << "Gas: " << db.get_cardinality("Gas") << endl;
	cout << endl;
	cout << "Radioactive: " << db.get_cardinality("Radioactive") << endl;
	cout << "Stable: " << db.get_cardinality("Stable") << endl;
	cout << endl;
	cout << "Natural: " << db.get_cardinality("Natural") << endl;
	cout << "Synthetic: " << db.get_cardinality("Synthetic") << endl;
	cout << endl;
	cout << "Ferromagnetic: " << db.get_cardinality("Ferromagnetic") << endl;
	cout << "Paramagnetic: " << db.get_cardinality("Paramagnetic") << endl;
	cout << "Diamagnetic: " << db.get_cardinality("Diamagnetic") << endl;
	cout << "Diatomic: " << db.get_cardinality("Diatomic") << endl;
	cout << endl;
	for (int g = 1; g <= 18; g++) {
		cout << "Group" << g << ": " << db.get_cardinality("Group" + to_string(g)) << "  ";
		if (g % 6 == 0) cout << endl;
	}
	cout << endl;

	// Sample element details
	cout << endl << "=== Sample: Iron ===" << endl;
	cout << db.list_properties("Iron") << endl;
	cout << endl;

	cout << "=== Diatomic Elements ===" << endl;
	cout << db.list_elements("Diatomic") << endl;
	cout << endl;

	cout << "=== Ferromagnetic Elements ===" << endl;
	cout << db.list_elements("Ferromagnetic") << endl;
	cout << endl;

	cout << "=== Group 1 (Alkali Metals + H) ===" << endl;
	cout << db.list_elements("Group1") << endl;
	cout << endl;

	cout << "=== Noble Gases ===" << endl;
	cout << db.list_elements("NobleGases") << endl;
	cout << endl;

	cout << "=== Liquid at room temperature ===" << endl;
	cout << db.list_elements("Liquid") << endl;

	return 0;
}
