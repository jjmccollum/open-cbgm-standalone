/*
 * compare_witnesses.cpp
 *
 *  Created on: Nov 20, 2019
 *      Author: jjmccollum
 */

#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <vector>
#include <set>

#include "cxxopts.hpp"
#include "roaring.hh"
#include "sqlite3.h"

#include "compare_witnesses_table.h"
#include "witness.h"

using namespace std;
using namespace roaring;

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
	set<string> acceptable_formats = set<string>({"fixed", "csv", "tsv", "json"});
	string format = "fixed";
	string output = "";
	string input_db_name = string();
	string primary_wit_id = string();
	set<string> secondary_wit_ids = set<string>();
	try {
		cxxopts::Options options("compare_witnesses", "Get a table of genealogical relationships relative to the witness with the given ID.\nOptionally, the user can specify one or more secondary witnesses, in which case the output will be restricted to the primary witness's relationships with those witnesses.");
		options.custom_help("[-h] [-f format] [-o output] input_db witness [secondary_witness_1 secondary witness_2 ...]");
		options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help")
				("f,format", "output format (must be one of {fixed, csv, tsv, json}; default is fixed)", cxxopts::value<string>())
				("o,output", "output file name (if not specified, output will be written to command line)", cxxopts::value<string>());
		options.add_options("positional")
				("input_db", "genealogical cache database", cxxopts::value<string>())
				("witness", "ID of the primary witness to be compared, as found in its <witness> element in the XML file", cxxopts::value<string>())
				("secondary_witnesses", "IDs of secondary witness to be compared to the primary witness (if not specified, then the primary witness will be compared to all other witnesses)", cxxopts::value<vector<string>>());
		options.parse_positional({"input_db", "witness", "secondary_witnesses"});
		auto args = options.parse(argc, argv);
		//Print help documentation and exit if specified:
		if (args.count("help")) {
			cout << options.help({"", "positional"}) << endl;
			exit(0);
		}
		//Parse the optional arguments:
		if (args.count("f")) {
			format = args["f"].as<string>();
			if (acceptable_formats.find(format) == acceptable_formats.end()) {
				cerr << "Error: " << format << " is not a valid format." << endl;
				exit(1);
			}
		}
		if (args.count("o")) {
			output = args["o"].as<string>();
		}
		//Parse the positional arguments:
		if (!args.count("input_db") || !args.count("witness")) {
			cerr << "Error: At least 2 positional arguments (input_db and witness) are required." << endl;
			exit(1);
		}
		else {
			input_db_name = args["input_db"].as<string>();
			primary_wit_id = args["witness"].as<string>();
		}
		if (args.count("secondary_witnesses")) {
			vector<string> secondary_witnesses = args["secondary_witnesses"].as<vector<string>>();
			for (string secondary_wit_id : secondary_witnesses) {
				secondary_wit_ids.insert(secondary_wit_id);
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
	cout << "Retrieving witness list..." << endl;
	//Retrieve all witness IDs in the order in which they occur in the table:
	list<string> list_wit = get_list_wit(input_db);
	cout << "Retrieving genealogical relationships for primary witness..." << endl;
	//Get the witness, if it exists:
	if (!witness_exists(input_db, primary_wit_id)) {
		cerr << "Error: there are no rows in the GENEALOGICAL_COMPARISONS table for witness " << primary_wit_id << "." << endl;
		exit(1);
	}
	witness wit = get_witness(input_db, primary_wit_id);
	//If there is a set of secondary witness IDs, then make sure they are all valid:
	for (string secondary_wit_id : secondary_wit_ids) {
		//The primary witness's ID should not occur again as a secondary witness:
		if (secondary_wit_id == primary_wit_id) {
			cerr << "Error: the primary witness ID should not be included in the list of secondary witnesses." << endl;
			exit(1);
		}
		//The secondary witness's ID should be present in the database:
		if (!witness_exists(input_db, secondary_wit_id)) {
			cerr << "Error: there are no rows in the GENEALOGICAL_COMPARISONS table for witness " << secondary_wit_id << "." << endl;
			exit(1);
		}
	}
	//Close the database:
	cout << "Closing database..." << endl;
	sqlite3_close(input_db);
	cout << "Database closed." << endl;
	//Then initialize the table:
	compare_witnesses_table table = compare_witnesses_table(wit, list_wit, secondary_wit_ids);
	//Then write to the appropriate output:
	if (output.empty()) {
		cout << "Writing to cout..." << endl;
		//If no output was specified, then write to cout:
		if (format == "fixed") {
			table.to_fixed_width(cout);
		} else if (format == "csv") {
			table.to_csv(cout);
		} else if (format == "tsv") {
			table.to_tsv(cout);
		} else if (format == "json") {
			table.to_json(cout);
		}
	} else {
		//Otherwise, write to the output file:
		fstream file;
		file.open(output, ios::out);
		if (format == "fixed") {
			table.to_fixed_width(file);
		} else if (format == "csv") {
			table.to_csv(file);
		} else if (format == "tsv") {
			table.to_tsv(file);
		} else if (format == "json") {
			table.to_json(file);
		}
		file.close();
	}
	exit(0);
}
