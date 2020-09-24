/*
 * print_local_stemma.cpp
 *
 *  Created on: Jan 13, 2020
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
#include <unordered_map>

#include "cxxopts.h"
#include "sqlite3.h"

#include "variation_unit.h"
#include "local_stemma.h"

using namespace std;

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
 * Entry point to the script.
 */
int main(int argc, char* argv[]) {
	//Read in the command-line options:
	bool print_weights = false;
	set<string> filter_vu_ids = set<string>();
	string input_db_name = string();
	try {
		cxxopts::Options options("print_local_stemma", "Print local stemma graphs to .dot output files. The output files will be placed in the \"local\" directory.");
		options.custom_help("[-h] [--weights] input_db [passages]");
		options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help")
				("weights", "print edge weights", cxxopts::value<bool>());
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
		//Parse the optional arguments:
		if (args.count("weights")) {
			print_weights = args["weights"].as<bool>();
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
	//Close the database:
	cout << "Closing database..." << endl;
	sqlite3_close(input_db);
	cout << "Database closed." << endl;
	cout << "Generating local stemmata..." << endl;
	//Create the directory to write files to:
	string local_dir = "local";
	create_dir(local_dir);
	//Now generate the graph for each local stemma:
	for (variation_unit vu : variation_units) {
		string vu_id = vu.get_id();
		local_stemma ls = vu.get_local_stemma();
		//Complete the path to this file:
		string filepath = local_dir + "/" + vu_id + "-local-stemma.dot";
		//Open a filestream:
		fstream dot_file;
		dot_file.open(filepath, ios::out);
		ls.to_dot(dot_file, print_weights);
		dot_file.close();
	}
	exit(0);
}
