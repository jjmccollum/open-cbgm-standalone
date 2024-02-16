/*
 * enumerate_relationships.cpp
 *
 *  Created on: Feb 16, 2024
 *      Author: jjmccollum
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <unordered_map>
#include <limits>

#include "cxxopts.hpp"
#include "roaring.hh"
#include "sqlite3.h"

#include "witness.h"
#include "variation_unit.h"
#include "local_stemma.h"

using namespace std;
using namespace roaring;

/**
 * Retrieves all rows from the VARIATION_UNITS table of the given SQLite database
 * and returns a vector of variation unit IDs populated with its contents.
 */
vector<string> get_variation_unit_ids(sqlite3 * input_db) {
	vector<string> variation_unit_ids = vector<string>();
	int rc; //to store SQLite macros
	sqlite3_stmt * select_from_variation_units_stmt;
	sqlite3_prepare(input_db, "SELECT VARIATION_UNIT FROM VARIATION_UNITS ORDER BY ROW_ID", -1, & select_from_variation_units_stmt, 0);
	rc = sqlite3_step(select_from_variation_units_stmt);
	while (rc == SQLITE_ROW) {
		string vu_id = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_variation_units_stmt, 0)));
		variation_unit_ids.push_back(vu_id);
		rc = sqlite3_step(select_from_variation_units_stmt);
	}
	sqlite3_finalize(select_from_variation_units_stmt);
	return variation_unit_ids;
}

/**
 * Retrieves all rows with the given witness ID from the GENEALOGICAL_COMPARISONS table of the given SQLite database
 * and returns a flag indicating whether such a witness exists.
 */
bool witness_exists(sqlite3 * input_db, const string & wit_id) {
	bool witness_exists = false;
	int rc; //to store SQLite macros
	sqlite3_stmt * select_from_genealogical_comparisons_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM GENEALOGICAL_COMPARISONS WHERE PRIMARY_WIT=? ORDER BY ROW_ID", -1, & select_from_genealogical_comparisons_stmt, 0);
	sqlite3_bind_text(select_from_genealogical_comparisons_stmt, 1, wit_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_genealogical_comparisons_stmt);
	while (rc == SQLITE_ROW) {
		witness_exists = true;
		break;
	}
	sqlite3_finalize(select_from_genealogical_comparisons_stmt);
	return witness_exists;
}

/**
 * Using a row for the given primary and secondary witness IDs from the GENEALOGICAL_COMPARISONS table of the given SQLite database,
 * returns a genealogical comparison.
 */
genealogical_comparison get_genealogical_comparison(sqlite3 * input_db, const string & primary_wit_id, const string & secondary_wit_id) {
	int rc; //to store SQLite macros
	//Populate this witness's list of genealogical comparisons to other witnesses:
	genealogical_comparison comp = genealogical_comparison();
	sqlite3_stmt * select_from_genealogical_comparisons_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM GENEALOGICAL_COMPARISONS WHERE PRIMARY_WIT=? AND SECONDARY_WIT=? ORDER BY ROW_ID", -1, & select_from_genealogical_comparisons_stmt, 0);
	sqlite3_bind_text(select_from_genealogical_comparisons_stmt, 1, primary_wit_id.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(select_from_genealogical_comparisons_stmt, 2, secondary_wit_id.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_genealogical_comparisons_stmt);
	while (rc == SQLITE_ROW) {
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
		break;
	}
	sqlite3_finalize(select_from_genealogical_comparisons_stmt);
	return comp;
}

/**
 * Entry point to the script.
 */
int main(int argc, char* argv[]) {
	//Read in the command-line options:
	set<string> acceptable_relationship_types = set<string>({"extant", "agree", "prior", "posterior", "norel", "unclear", "explained"});
	string output = "";
	string input_db_name = string();
	string primary_wit_id = string();
	string secondary_wit_id = string();
	set<string> filter_relationship_types = set<string>();
	try {
		cxxopts::Options options("enumerate_relationships", "Get a printout of all variation units where the two witnesses with specified IDs have one or more given types of genealogical relationships.\nIf no types of genealogical relationships are specified, then the variation units for each type of relationship are enumerated separately.");
		options.custom_help("[-h] input_db primary_witness secondary_witness [relationship_type_1 relationship_type_2 ...]");
		options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help");
		options.add_options("positional")
				("input_db", "genealogical cache database", cxxopts::value<string>())
				("primary_witness", "ID of the primary witness to be checked, as found in its <witness> element in the XML file", cxxopts::value<string>())
				("secondary_witness", "ID of the secondary witness to be checked, as found in its <witness> element in the XML file", cxxopts::value<string>())
				("relationship_types", "desired genealogical relationship types (acceptable values are {extant, agree, prior, posterior, norel, unclear, explained}); if none are specified, variation units for all types of relationships will be enumerated", cxxopts::value<vector<string>>());
		options.parse_positional({"input_db", "primary_witness", "secondary_witness", "relationship_types"});
		auto args = options.parse(argc, argv);
		//Print help documentation and exit if specified:
		if (args.count("help")) {
			cout << options.help({"", "positional"}) << endl;
			exit(0);
		}
		//Parse the positional arguments:
		if (!args.count("input_db") || !args.count("primary_witness") || !args.count("secondary_witness")) {
			cerr << "Error: At least 3 positional arguments (input_db, primary_witness, and secondary_witness) are required." << endl;
			exit(1);
		}
		else {
			input_db_name = args["input_db"].as<string>();
			primary_wit_id = args["primary_witness"].as<string>();
			secondary_wit_id = args["secondary_witness"].as<string>();
		}
		if (args.count("relationship_types")) {
			vector<string> relationship_types = args["relationship_types"].as<vector<string>>();
			for (string relationship_type : relationship_types) {
				if (acceptable_relationship_types.find(relationship_type) == acceptable_relationship_types.end()) {
					cerr << "Error: " << relationship_type << " is not a valid genealogical relationship type." << endl;
					exit(1);
				}
				filter_relationship_types.insert(relationship_type);
			}
		} else {
			//If no relationship types are specified, then look for all of them:
			filter_relationship_types = set<string>(acceptable_relationship_types);
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
	//Retrieve all variation unit IDs in the order in which they occur in the table:
	vector<string> variation_unit_ids = get_variation_unit_ids(input_db);
	cout << "Retrieving genealogical comparison between primary witness and secondary witness..." << endl;
	//Get the genealogical relationship, if both witnesses exist:
	if (!witness_exists(input_db, primary_wit_id)) {
		cerr << "Error: there are no rows in the GENEALOGICAL_COMPARISONS table for witness " << primary_wit_id << "." << endl;
		exit(1);
	}
	if (!witness_exists(input_db, secondary_wit_id)) {
		cerr << "Error: there are no rows in the GENEALOGICAL_COMPARISONS table for witness " << secondary_wit_id << "." << endl;
		exit(1);
	}
	genealogical_comparison comp = get_genealogical_comparison(input_db, primary_wit_id, secondary_wit_id);
	//Close the database:
	cout << "Closing database..." << endl;
	sqlite3_close(input_db);
	cout << "Database closed." << endl;
	//Then print out the variation units corresponding to the set bits in the desired relationship types in the genealogical comparison:
	cout << "Genealogical relationships between " << primary_wit_id << " and " << secondary_wit_id << ":\n" << endl;
	if (filter_relationship_types.find("extant") != filter_relationship_types.end()) {
		uint64_t extant_cardinality = comp.extant.cardinality();
		uint32_t * extant_vu_indices = new uint32_t[extant_cardinality];
		comp.extant.toUint32Array(extant_vu_indices);
		cout << "EXTANT" << endl;
		for (uint32_t i = 0; i < extant_cardinality; i++) {
			cout << "\t" << variation_unit_ids[extant_vu_indices[i]] << endl;
		}
		delete[] extant_vu_indices;
	}
	if (filter_relationship_types.find("agree") != filter_relationship_types.end()) {
		uint64_t agreements_cardinality = comp.agreements.cardinality();
		uint32_t * agreements_vu_indices = new uint32_t[agreements_cardinality];
		comp.agreements.toUint32Array(agreements_vu_indices);
		cout << "AGREE" << endl;
		for (uint32_t i = 0; i < agreements_cardinality; i++) {
			cout << "\t" << variation_unit_ids[agreements_vu_indices[i]] << endl;
		}
		delete[] agreements_vu_indices;
	}
	if (filter_relationship_types.find("prior") != filter_relationship_types.end()) {
		uint64_t prior_cardinality = comp.prior.cardinality();
		uint32_t * prior_vu_indices = new uint32_t[prior_cardinality];
		comp.prior.toUint32Array(prior_vu_indices);
		cout << "PRIOR" << endl;
		for (uint32_t i = 0; i < prior_cardinality; i++) {
			cout << "\t" << variation_unit_ids[prior_vu_indices[i]] << endl;
		}
		delete[] prior_vu_indices;
	}
	if (filter_relationship_types.find("posterior") != filter_relationship_types.end()) {
		uint64_t posterior_cardinality = comp.posterior.cardinality();
		uint32_t * posterior_vu_indices = new uint32_t[posterior_cardinality];
		comp.posterior.toUint32Array(posterior_vu_indices);
		cout << "POSTERIOR" << endl;
		for (uint32_t i = 0; i < posterior_cardinality; i++) {
			cout << "\t" << variation_unit_ids[posterior_vu_indices[i]] << endl;
		}
		delete[] posterior_vu_indices;
	}
	if (filter_relationship_types.find("norel") != filter_relationship_types.end()) {
		uint64_t norel_cardinality = comp.norel.cardinality();
		uint32_t * norel_vu_indices = new uint32_t[norel_cardinality];
		comp.norel.toUint32Array(norel_vu_indices);
		cout << "NOREL" << endl;
		for (uint32_t i = 0; i < norel_cardinality; i++) {
			cout << "\t" << variation_unit_ids[norel_vu_indices[i]] << endl;
		}
		delete[] norel_vu_indices;
	}
	if (filter_relationship_types.find("unclear") != filter_relationship_types.end()) {
		uint64_t unclear_cardinality = comp.unclear.cardinality();
		uint32_t * unclear_vu_indices = new uint32_t[unclear_cardinality];
		comp.unclear.toUint32Array(unclear_vu_indices);
		cout << "UNCLEAR" << endl;
		for (uint32_t i = 0; i < unclear_cardinality; i++) {
			cout << "\t" << variation_unit_ids[unclear_vu_indices[i]] << endl;
		}
		delete[] unclear_vu_indices;
	}
	if (filter_relationship_types.find("explained") != filter_relationship_types.end()) {
		uint64_t explained_cardinality = comp.explained.cardinality();
		uint32_t * explained_vu_indices = new uint32_t[explained_cardinality];
		comp.explained.toUint32Array(explained_vu_indices);
		cout << "EXPLAINED" << endl;
		for (uint32_t i = 0; i < explained_cardinality; i++) {
			cout << "\t" << variation_unit_ids[explained_vu_indices[i]] << endl;
		}
		delete[] explained_vu_indices;
	}
	exit(0);
}
