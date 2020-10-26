/*
 * optimize_substemmata.cpp
 *
 *  Created on: Dec 2, 2019
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

#include "optimize_substemmata_table.h"
#include "witness.h"
#include "variation_unit.h"

using namespace std;

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
 * Retrieves all rows from the VARIATION_UNITS table of the given SQLite database
 * and returns a vector of variation unit labels populated with its contents.
 */
vector<string> get_variation_unit_labels(sqlite3 * input_db) {
	vector<string> variation_unit_labels = vector<string>();
	int rc; //to store SQLite macros
	sqlite3_stmt * select_from_variation_units_stmt;
	sqlite3_prepare(input_db, "SELECT LABEL FROM VARIATION_UNITS ORDER BY ROW_ID", -1, & select_from_variation_units_stmt, 0);
	rc = sqlite3_step(select_from_variation_units_stmt);
	while (rc == SQLITE_ROW) {
		string label = string(reinterpret_cast<const char *>(sqlite3_column_text(select_from_variation_units_stmt, 0)));
		variation_unit_labels.push_back(label);
		rc = sqlite3_step(select_from_variation_units_stmt);
	}
	sqlite3_finalize(select_from_variation_units_stmt);
	return variation_unit_labels;
}

/**
 * Entry point to the script.
 */
int main(int argc, char* argv[]) {
	//Read in the command-line options:
	float fixed_ub = 0;
	set<string> acceptable_formats = set<string>({"fixed", "csv", "tsv", "json"});
	string format = "fixed";
	string output = "";
	string input_db_name = string();
	string wit_id = string();
	try {
		cxxopts::Options options("optimize_substemmata", "Get a table of best-found substemmata for the witness with the given ID.\nOptionally, the user can specify an upper bound on substemma cost, in which case the output will enumerate all substemmata within the cost bound.");
		options.custom_help("[-h] [-b bound] [-f format] [-o output] input_db witness");
		options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help")
				("b,bound", "fixed upper bound on substemmata cost; if specified, list all substemmata with costs within this bound", cxxopts::value<float>())
				("f,format", "output format (must be one of {fixed, csv, tsv, json}; default is fixed)", cxxopts::value<string>())
				("o,output", "output file name (if not specified, output will be written to command line)", cxxopts::value<string>());
		options.add_options("positional")
				("input_db", "genealogical cache database", cxxopts::value<string>())
				("witness", "ID of the witness whose relatives are desired, as found in its <witness> element in the XML file", cxxopts::value<vector<string>>());
		options.parse_positional({"input_db", "witness"});
		auto args = options.parse(argc, argv);
		//Print help documentation and exit if specified:
		if (args.count("help")) {
			cout << options.help({"", "positional"}) << endl;
			exit(0);
		}
		//Parse the optional arguments:
		if (args.count("b")) {
			fixed_ub = args["b"].as<float>();
		}
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
		if (!args.count("input_db") || args.count("witness") != 1) {
			cerr << "Error: 2 positional arguments (input_db and witness) are required." << endl;
			exit(1);
		}
		else {
			input_db_name = args["input_db"].as<string>();
			wit_id = args["witness"].as<vector<string>>()[0];
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
	cout << "Retrieving genealogical relationships for witness..." << endl;
	//Get the witness, if it exists:
	if (!witness_exists(input_db, wit_id)) {
		cerr << "Error: there are no rows in the GENEALOGICAL_COMPARISONS table for witness " << wit_id << "." << endl;
		exit(1);
	}
	witness wit = get_witness(input_db, wit_id);
	cout << "Retrieving variation unit labels..." << endl;
	vector<string> vu_labels = get_variation_unit_labels(input_db);
	//Close the database:
	cout << "Closing database..." << endl;
	sqlite3_close(input_db);
	cout << "Database closed." << endl;
	//If the witness has no potential ancestors, then let the user know:
	if (wit.get_potential_ancestor_ids().empty()) {
		cout << "The witness with ID " << wit_id << " has no potential ancestors. This may be because it is too fragmentary or because it has equal priority to the Ausgangstext according to local stemmata." << endl;
		exit(0);
	}
	//Otherwise, inform the user of whether we'll be searching for minimum-cost substemmata or enumerating all substemmata within a cost bound:
	if (fixed_ub > 0) {
		cout << "Finding all substemmata for witness " << wit_id << " with costs within " << fixed_ub << "..." << endl;
	}
	else {
		cout << "Finding minimum-cost substemmata for witness " << wit_id << "..." << endl;
	}
	//Then initialize the table:
	optimize_substemmata_table table = optimize_substemmata_table(wit, fixed_ub);
	//If the table is empty, then find out why:
	if (table.get_rows().empty()) {
		//Check if the set cover problem is infeasible:
		Roaring extant = wit.get_genealogical_comparison_for_witness(wit_id).extant;
		Roaring covered = Roaring();
		for (string potential_ancestor_id : wit.get_potential_ancestor_ids()) {
			covered |= wit.get_genealogical_comparison_for_witness(potential_ancestor_id).explained;
		}
		Roaring uncovered = extant ^ covered;
		//If there are any passages not covered, then report them:
		if (!uncovered.isEmpty()) {
			cout << "The witness with ID " << wit_id << " cannot be explained by any of its potential ancestors at the following variation units: ";
			for (Roaring::const_iterator it = uncovered.begin(); it != uncovered.end(); it++) {
				unsigned int col_ind = *it;
				string vu_label = vu_labels[col_ind];
				if (it != uncovered.begin()) {
					cout << ", ";
				}
				cout << vu_label;
			}
			cout << endl;
			exit(0);
		}
		//Otherwise, if a fixed upper bound was specified, and no solution was found, then tell the user to raise the upper bound:
		if (fixed_ub > 0) {
			cout << "No substemma exists with a cost below " << fixed_ub << "; try again with a higher bound or without specifying a fixed upper bound." << endl;
			exit(0);
		}
	}
	//Then write to the appropriate output:
	if (output.empty()) {
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
