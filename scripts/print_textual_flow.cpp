/*
 * print_textual_flow.cpp
 *
 *  Created on: Jan 14, 2020
 *      Author: jjmccollum
 */

#ifdef _WIN32
	#include <direct.h> //for Windows _mkdir() support
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <tuple>
#include <unordered_map>
#include <limits>

#include "cxxopts.hpp"
#include "roaring.hh"
#include "sqlite3.h"
#include "local_stemma.h"
#include "variation_unit.h"
#include "witness.h"
#include "textual_flow.h"


using namespace std;
using namespace roaring;

/**
 * Creates a directory with the given name.
 * The return value will be 0 if successful, and 1 otherwise.
 */
int create_dir(const string & dir) {
	#ifdef _WIN32
		return _mkdir(dir.c_str());
	#else
		umask(0); //this is done to ensure that the newly created directory will have exactly the permissions we specify below
		return mkdir(dir.c_str(), 0755);
	#endif
}

/**
 * Retrieves all rows from the VARIATION_UNITS table of the given SQLite database
 * and returns a list of variation_unit IDs populated with its contents.
 */
list<string> get_variation_unit_ids(sqlite3 * input_db) {
	list<string> variation_unit_ids = list<string>();
	int rc; //to store SQLite macros
	sqlite3_stmt * select_from_variation_units_stmt;
	sqlite3_prepare(input_db, "SELECT VARIATION_UNIT FROM VARIATION_UNITS ORDER BY ROW_ID", -1, & select_from_variation_units_stmt, 0);
	rc = sqlite3_step(select_from_variation_units_stmt);
	while (rc == SQLITE_ROW) {
		string id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_variation_units_stmt, 0)));
		variation_unit_ids.push_back(id);
		rc = sqlite3_step(select_from_variation_units_stmt);
	}
	sqlite3_finalize(select_from_variation_units_stmt);
	return variation_unit_ids;
}

/**
 * Retrieves all rows with the given variation unit ID from the VARIATION_UNITS table of the given SQLite database
 * and returns a flag indicating whether such a variation unit exists.
 */
bool variation_unit_exists(sqlite3 * input_db, const string & vu_id) {
	bool variation_unit_exists = false;
	int rc; //to store SQLite macros
	sqlite3_stmt * select_from_variation_units_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM VARIATION_UNITS WHERE VARIATION_UNIT=?", -1, & select_from_variation_units_stmt, 0);
	sqlite3_bind_text(select_from_variation_units_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_variation_units_stmt);
	while (rc == SQLITE_ROW) {
		variation_unit_exists = true;
		break;
	}
	sqlite3_finalize(select_from_variation_units_stmt);
	return variation_unit_exists;
}

/**
 * Using rows for the given variation unit ID from the VARIATION_UNITS, READINGS, READING_RELATIONS, and READING_SUPPORT tables of the given SQLite database,
 * returns a variation unit.
 */
variation_unit get_variation_unit(sqlite3 * input_db, const string & vu_id) {
	int rc; //to store SQLite macros
	//Retrieve the variation unit's label and connectivity limit:
	string label = "";
	int connectivity = 0;
	sqlite3_stmt * select_from_variation_units_stmt;
	sqlite3_prepare(input_db, "SELECT LABEL, CONNECTIVITY FROM VARIATION_UNITS WHERE VARIATION_UNIT=?", -1, & select_from_variation_units_stmt, 0);
	sqlite3_bind_text(select_from_variation_units_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_variation_units_stmt);
	while (rc == SQLITE_ROW) {
		label = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_variation_units_stmt, 0)));
		connectivity = int(sqlite3_column_int(select_from_variation_units_stmt, 1));
		break;
	}
	sqlite3_finalize(select_from_variation_units_stmt);
	//Populate a list of readings and a list of vertices for this unit's local stemma:
	list<string> readings = list<string>();
	list<local_stemma_vertex> vertices = list<local_stemma_vertex>();
	sqlite3_stmt * select_from_readings_stmt;
	sqlite3_prepare(input_db, "SELECT READING FROM READINGS WHERE VARIATION_UNIT=? ORDER BY ROW_ID", -1, & select_from_readings_stmt, 0);
	sqlite3_bind_text(select_from_readings_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_readings_stmt);
	while (rc == SQLITE_ROW) {
		string rdg = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_readings_stmt, 0)));
		readings.push_back(rdg);
		local_stemma_vertex v;
		v.id = rdg;
		vertices.push_back(v);
		rc = sqlite3_step(select_from_readings_stmt);
	}
	sqlite3_finalize(select_from_readings_stmt);
	//Populate a list of edges for this unit's local_stemma:
	list<local_stemma_edge> edges = list<local_stemma_edge>();
	sqlite3_stmt * select_from_reading_relations_stmt;
	sqlite3_prepare(input_db, "SELECT PRIOR, POSTERIOR, WEIGHT FROM READING_RELATIONS WHERE VARIATION_UNIT=? ORDER BY ROW_ID", -1, & select_from_reading_relations_stmt, 0);
	sqlite3_bind_text(select_from_reading_relations_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_reading_relations_stmt);
	while (rc == SQLITE_ROW) {
		local_stemma_edge e;
		e.prior = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_reading_relations_stmt, 0)));
		e.posterior = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_reading_relations_stmt, 1)));
		e.weight = float(sqlite3_column_double(select_from_reading_relations_stmt, 2));
		edges.push_back(e);
		rc = sqlite3_step(select_from_reading_relations_stmt);
	}
	sqlite3_finalize(select_from_reading_relations_stmt);
	//Construct the local stemma for this unit:
	local_stemma ls = local_stemma(vu_id, label, vertices, edges);
	//Construct the reading support map for this unit:
	unordered_map<string, string> reading_support = unordered_map<string, string>();
	sqlite3_stmt * select_from_reading_support_stmt;
	sqlite3_prepare(input_db, "SELECT WITNESS, READING FROM READING_SUPPORT WHERE VARIATION_UNIT=? ORDER BY ROW_ID", -1, & select_from_reading_support_stmt, 0);
	sqlite3_bind_text(select_from_reading_support_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_reading_support_stmt);
	while (rc == SQLITE_ROW) {
		string wit_id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_reading_support_stmt, 0)));
		string rdg = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_reading_support_stmt, 1)));
		reading_support[wit_id] = rdg;
		rc = sqlite3_step(select_from_reading_support_stmt);
	}
	sqlite3_finalize(select_from_reading_support_stmt);
	//Then construct this variation unit:
	variation_unit vu = variation_unit(vu_id, label, readings, reading_support, connectivity, ls);
	return vu;
}

/**
 * Retrieves all rows from the WITNESSES table of the given SQLite database
 * and returns a list of witness IDs populated with its contents.
 */
list<string> get_list_wit(sqlite3 * input_db) {
	list<string> list_wit = list<string>();
	int rc; //to store SQLite macros
	sqlite3_stmt * select_from_witnesses_stmt;
	sqlite3_prepare(input_db, "SELECT WITNESS FROM WITNESSES ORDER BY ROW_ID", -1, & select_from_witnesses_stmt, 0);
	rc = sqlite3_step(select_from_witnesses_stmt);
	while (rc == SQLITE_ROW) {
		string wit_id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_witnesses_stmt, 0)));
		list_wit.push_back(wit_id);
		rc = sqlite3_step(select_from_witnesses_stmt);
	}
	sqlite3_finalize(select_from_witnesses_stmt);
	return list_wit;
}

/**
 * Using rows for the given witness ID from the GENEALOGICAL_COMPARISONS table of the given SQLite database,
 * returns a witness.
 */
witness get_witness(sqlite3 * input_db, const string & wit_id) {
	int rc; //to store SQLite macros
	//Populate this witness's list of genealogical comparisons to other witnesses:
	list<genealogical_comparison> comps = list<genealogical_comparison>();
	sqlite3_stmt * select_from_genealogical_comparisons_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM GENEALOGICAL_COMPARISONS WHERE PRIMARY_WIT=? ORDER BY ROW_ID", -1, & select_from_genealogical_comparisons_stmt, 0);
	sqlite3_bind_text(select_from_genealogical_comparisons_stmt, 1, wit_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_genealogical_comparisons_stmt);
	while (rc == SQLITE_ROW) {
		genealogical_comparison comp;
		string primary_wit_id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_genealogical_comparisons_stmt, 1)));
		comp.primary_wit = primary_wit_id;
		string secondary_wit_id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_genealogical_comparisons_stmt, 2)));
		comp.secondary_wit = secondary_wit_id;
		int extant_bytes = sqlite3_column_bytes(select_from_genealogical_comparisons_stmt, 3);
		const char * extant_buf = reinterpret_cast<const char *>(sqlite3_column_blob(select_from_genealogical_comparisons_stmt, 3));
		Roaring extant = Roaring::readSafe(extant_buf, extant_bytes);
		comp.extant = extant;
		int agreements_bytes = sqlite3_column_bytes(select_from_genealogical_comparisons_stmt, 4);
		const char * agreements_buf = reinterpret_cast<const char *>(sqlite3_column_blob(select_from_genealogical_comparisons_stmt, 4));
		Roaring agreements = Roaring::readSafe(agreements_buf, agreements_bytes);
		comp.agreements = agreements;
		int prior_bytes = sqlite3_column_bytes(select_from_genealogical_comparisons_stmt, 5);
		const char * prior_buf = reinterpret_cast<const char *>(sqlite3_column_blob(select_from_genealogical_comparisons_stmt, 5));
		Roaring prior = Roaring::readSafe(prior_buf, prior_bytes);
		comp.prior = prior;
		int posterior_bytes = sqlite3_column_bytes(select_from_genealogical_comparisons_stmt, 6);
		const char * posterior_buf = reinterpret_cast<const char *>(sqlite3_column_blob(select_from_genealogical_comparisons_stmt, 6));
		Roaring posterior = Roaring::readSafe(posterior_buf, posterior_bytes);
		comp.posterior = posterior;
		int norel_bytes = sqlite3_column_bytes(select_from_genealogical_comparisons_stmt, 7);
		const char * norel_buf = reinterpret_cast<const char *>(sqlite3_column_blob(select_from_genealogical_comparisons_stmt, 7));
		Roaring norel = Roaring::readSafe(norel_buf, norel_bytes);
		comp.norel = norel;
		int unclear_bytes = sqlite3_column_bytes(select_from_genealogical_comparisons_stmt, 8);
		const char * unclear_buf = reinterpret_cast<const char *>(sqlite3_column_blob(select_from_genealogical_comparisons_stmt, 8));
		Roaring unclear = Roaring::readSafe(unclear_buf, unclear_bytes);
		comp.unclear = unclear;
		int explained_bytes = sqlite3_column_bytes(select_from_genealogical_comparisons_stmt, 9);
		const char * explained_buf = reinterpret_cast<const char *>(sqlite3_column_blob(select_from_genealogical_comparisons_stmt, 9));
		Roaring explained = Roaring::readSafe(explained_buf, explained_bytes);
		comp.explained = explained;
		float cost = float(sqlite3_column_double(select_from_genealogical_comparisons_stmt, 10));
		comp.cost = cost;
		comps.push_back(comp);
		rc = sqlite3_step(select_from_genealogical_comparisons_stmt);
	}
	sqlite3_finalize(select_from_genealogical_comparisons_stmt);
	witness wit = witness(wit_id, comps);
	return wit;
}

/**
 * Entry point to the script.
 */
int main(int argc, char* argv[]) {
	//Read in the command-line options:
	bool flow = false;
	bool attestations = false;
	bool variants = false;
	bool flow_strengths = false;
	int connectivity = -1;
	set<string> filter_vu_ids = set<string>();
	string input_db_name = string();
	try {
		cxxopts::Options options("print_textual_flow", "Prints multiple types of textual flow diagrams to .dot output files. The output files will be placed in the \"flow\", \"attestations\", and \"variants\" directories.");
		options.custom_help("[-h] [--flow] [--attestations] [--variants] [--strengths] [-k connectivity] input_db [passages]");
		options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help")
				("flow", "print complete textual flow diagrams", cxxopts::value<bool>())
				("attestations", "print coherence in attestation textual flow diagrams", cxxopts::value<bool>())
				("variants", "print coherence at variant passages diagrams (i.e., textual flow diagrams restricted to flow between different readings)", cxxopts::value<bool>())
				("strengths", "format edges to reflect flow strengths", cxxopts::value<bool>())
				("k,connectivity", "desired connectivity limit (if not specified, default value in database is used)", cxxopts::value<int>());
		options.add_options("positional")
				("input_db", "genealogical cache database", cxxopts::value<string>())
				("passages", "if specified, only print graphs for the variation units with the given IDs; otherwise, print graphs for all variation units", cxxopts::value<vector<string>>());
		options.parse_positional({"input_db", "passages"});
		auto args = options.parse(argc, argv);
		//Print help documentation and exit if specified:
		if (args.count("help")) {
			cout << options.help({"", "positional"}) << endl;
			exit(0);
		}
		if (args.count("flow")) {
			flow = args["flow"].as<bool>();
		}
		if (args.count("attestations")) {
			attestations = args["attestations"].as<bool>();
		}
		if (args.count("variants")) {
			variants = args["variants"].as<bool>();
		}
		//If no specific graph types are specified, then print all of them:
		if (!(flow || attestations || variants)) {
			flow = true;
			attestations = true;
			variants = true;
		}
		//Parse the optional arguments:
		if (args.count("strengths")) {
			flow_strengths = args["strengths"].as<bool>();
		}
		if (args.count("k")) {
			connectivity = args["k"].as<int>();
			if (connectivity <= 0) {
				cerr << "Error: connectivity (argument -k) must be a positive integer." << endl;
				exit(1);
			}
		}
		//Parse the positional arguments:
		if (!args.count("input_db")) {
			cerr << "Error: 1 positional argument (input_db) is required." << endl;
			exit(1);
		}
		else {
			input_db_name = args["input_db"].as<string>();
		}
		if (args.count("passages")) {
			vector<string> passages = args["passages"].as<vector<string>>();
			for (string filter_vu_id : passages) {
				filter_vu_ids.insert(filter_vu_id);
			}
		}
	}
	catch (const cxxopts::OptionException & e) {
		cerr << "Error parsing options: " << e.what() << endl;
		exit(-1);
	}
	//Open the database:
	cout << "Opening database..." << endl;
	sqlite3 * input_db;
	int rc = sqlite3_open(input_db_name.c_str(), & input_db);
	if (rc) {
		cerr << "Error opening database " << input_db_name << ": " << sqlite3_errmsg(input_db) << endl;
		exit(1);
	}
	cout << "Retrieving variation unit list..." << endl;
	//Retrieve a list of all variation unit IDs:
	list<string> variation_unit_ids = get_variation_unit_ids(input_db);
	//If a filter set of variation unit IDs was specified, then filter this list:
	if (!filter_vu_ids.empty()) {
		//First, make sure every ID in the filter set corresponds to an existing variation unit:
		for (string vu_id : filter_vu_ids) {
			if (!variation_unit_exists(input_db, vu_id)) {
				cerr << "Error: there are no rows in the VARIATION_UNITS table for variation unit ID " << vu_id << "." << endl;
				exit(1);
			}
		}
		variation_unit_ids.remove_if([&](const string & id) {
			return filter_vu_ids.find(id) == filter_vu_ids.end();
		});
	}
	cout << "Retrieving variation unit(s)..." << endl;
	//Then, for each variation unit ID in the list, get the corresponding variation unit:
	list<variation_unit> variation_units = list<variation_unit>();
	for (string vu_id : variation_unit_ids) {
		variation_unit vu = get_variation_unit(input_db, vu_id);
		variation_units.push_back(vu);
	}
	cout << "Retrieving witness list..." << endl;
	//Retrieve all witness IDs in the order in which they occur in the table:
	list<string> list_wit = get_list_wit(input_db);
	cout << "Initializing all witnesses..." << endl;
	//Populate a list of witnesses:
	list<witness> witnesses = list<witness>();
	for (string wit_id : list_wit) {
		witness wit = get_witness(input_db, wit_id);
		witnesses.push_back(wit);
	}
	//Close the database:
	cout << "Closing database..." << endl;
	sqlite3_close(input_db);
	cout << "Database closed." << endl;
	cout << "Generating textual flow diagrams..." << endl;
	//Now generate the graphs for each variation unit:
	for (variation_unit vu : variation_units) {
		string vu_id = vu.get_id();
		//Construct the underlying textual flow data structure using this variation unit, the list of witnesses, and, if specified, the connectivity:
		textual_flow tf = connectivity == -1 ? textual_flow(vu, witnesses) : textual_flow(vu, witnesses, connectivity);
		if (flow) {
			//Create the directory:
			string flow_dir = "flow";
			create_dir(flow_dir);
			//Complete the path to the file:
			string filepath = flow_dir + "/" + vu_id + "-textual-flow.dot";
			//Then write to file:
			fstream dot_file;
			dot_file.open(filepath, ios::out);
			tf.textual_flow_to_dot(dot_file, flow_strengths);
			dot_file.close();
		}
		if (attestations) {
			//Create the directory:
			string attestations_dir = "attestations";
			create_dir(attestations_dir);
			//A separate coherence in attestations diagram is drawn for each reading:
			for (string rdg : vu.get_readings()) {
				//Complete the path to the file:
				string filepath = attestations_dir + "/" + vu_id + "R" + rdg + "-coherence-attestations.dot";
				//Then write to file:
				fstream dot_file;
				dot_file.open(filepath, ios::out);
				tf.coherence_in_attestations_to_dot(dot_file, rdg, flow_strengths);
				dot_file.close();
			}
		}
		if (variants) {
			//Create the directory:
			string variants_dir = "variants";
			create_dir(variants_dir);
			//Complete the path to the file:
			string filepath = variants_dir + "/" + vu_id + "-coherence-variants.dot";
			//Then write to file:
			fstream dot_file;
			dot_file.open(filepath, ios::out);
			tf.coherence_in_variant_passages_to_dot(dot_file, flow_strengths);
			dot_file.close();
		}
	}
	exit(0);
}
