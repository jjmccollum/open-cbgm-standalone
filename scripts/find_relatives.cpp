/*
 * find_relatives.cpp
 *
 *  Created on: Nov 26, 2019
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

#include "find_relatives_table.h"
#include "witness.h"
#include "variation_unit.h"
#include "local_stemma.h"

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
 * Retrieves all rows with the given variation unit ID and reading ID from the READINGS table of the given SQLite database
 * and returns a flag indicating whether that reading exists at that variation unit.
 */
bool reading_exists(sqlite3 * input_db, const string & vu_id, const string & rdg) {
	bool reading_exists = false;
	int rc; //to store SQLite macros
	sqlite3_stmt * select_from_readings_stmt;
	sqlite3_prepare(input_db, "SELECT * FROM READINGS WHERE VARIATION_UNIT=? AND READING=?", -1, & select_from_readings_stmt, 0);
	sqlite3_bind_text(select_from_readings_stmt, 1, vu_id.c_str(), -1, SQLITE_STATIC);
	sqlite3_bind_text(select_from_readings_stmt, 2, rdg.c_str(), -1, SQLITE_STATIC);
	rc = sqlite3_step(select_from_readings_stmt);
	while (rc == SQLITE_ROW) {
		reading_exists = true;
		break;
	}
	sqlite3_finalize(select_from_readings_stmt);
	return reading_exists;
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
 * Entry point to the script.
 */
int main(int argc, char* argv[]) {
	//Read in the command-line options:
	set<string> acceptable_formats = set<string>({"fixed", "csv", "tsv", "json"});
	string format = "fixed";
	string output = "";
	string input_db_name = string();
	string primary_wit_id = string();
	string vu_id = string();
	set<string> filter_readings = set<string>();
	try {
		cxxopts::Options options("find_relatives", "Get a table of genealogical relationships between the witness with the given ID and other witnesses at a given passage, as specified by the user.\nOptionally, the user can specify one or more reading IDs for the given passage, in which case the output will be restricted to the witnesses preserving those readings.");
		options.custom_help("[-h] [-f format] [-o output] input_db witness passage [reading_1 reading_2 ...]");
		options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help")
				("f,format", "output format (must be one of {fixed, csv, tsv, json}; default is fixed)", cxxopts::value<string>())
				("o,output", "output file name (if not specified, output will be written to command line)", cxxopts::value<string>());
		options.add_options("positional")
				("input_db", "genealogical cache database", cxxopts::value<string>())
				("witness", "ID of the witness whose relatives are desired, as found in its <witness> element in the XML file", cxxopts::value<string>())
				("passage", "ID of the variation unit at which relatives' readings are desired", cxxopts::value<string>())
				("readings", "IDs of desired variant readings", cxxopts::value<vector<string>>());
		options.parse_positional({"input_db", "witness", "passage", "readings"});
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
		if (!args.count("input_db") || !args.count("witness") || !args.count("passage")) {
			cerr << "Error: At least 3 positional arguments (input_db, witness, and passage) are required." << endl;
			exit(1);
		}
		else {
			input_db_name = args["input_db"].as<string>();
			primary_wit_id = args["witness"].as<string>();
			vu_id = args["passage"].as<string>();
		}
		if (args.count("readings")) {
			vector<string> readings = args["readings"].as<vector<string>>();
			for (string reading : readings) {
				filter_readings.insert(reading);
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
	cout << "Retrieving variation unit..." << endl;
	//Get the variation unit, if it exists:
	if (!variation_unit_exists(input_db, vu_id)) {
		cerr << "Error: there are no rows in the VARIATION_UNITS table for variation unit ID " << vu_id << "." << endl;
		exit(1);
	}
	variation_unit vu = get_variation_unit(input_db, vu_id);
	//If there is a set of filter readings, then make sure that they all occur in this unit:
	for (string rdg : filter_readings) {
		if (!reading_exists(input_db, vu_id, rdg)) {
			cerr << "Error: there are no rows in the READINGS table for variation unit ID " << vu_id << " and reading ID " << rdg << "." << endl;
		    exit(1);
		}
	}
	//Close the database:
	cout << "Closing database..." << endl;
	sqlite3_close(input_db);
	cout << "Database closed." << endl;
	//Then initialize the table:
	find_relatives_table table = find_relatives_table(wit, vu, list_wit, filter_readings);
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
