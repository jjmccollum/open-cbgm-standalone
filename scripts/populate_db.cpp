/*
 * populate_db.cpp
 *
 *  Created on: Jan 10, 2020
 *      Author: jjmccollum
 */

#include <iostream>
#include <string>
#include <list>
#include <vector>
#include <set>
#include <unordered_map>

#include "cxxopts.hpp"
#include "pugixml.hpp"
#include "roaring.hh"
#include "sqlite3.h"
#include "witness.h"
#include "apparatus.h"
#include "variation_unit.h"
#include "local_stemma.h"


using namespace std;
using namespace pugi;
using namespace roaring;

/**
 * Creates, indexes, and populates the READINGS table.
 */
void populate_readings_table(sqlite3 * output_db, const apparatus & app) {
	int rc; //to store SQLite macros
	//Create the READINGS table:
	string create_readings_sql = "DROP TABLE IF EXISTS READINGS;"
			"CREATE TABLE READINGS ("
			"ROW_ID INT NOT NULL, "
			"VARIATION_UNIT TEXT NOT NULL, "
			"READING TEXT NOT NULL);";
	char * create_readings_error_msg;
	rc = sqlite3_exec(output_db, create_readings_sql.c_str(), NULL, 0, & create_readings_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating table READINGS: " << create_readings_error_msg << endl;
		sqlite3_free(create_readings_error_msg);
		exit(1);
	}
	//Denormalize it:
	string create_readings_idx_sql = "DROP INDEX IF EXISTS READINGS_IDX;"
			"CREATE INDEX READINGS_IDX ON READINGS (VARIATION_UNIT, READING);";
	char * create_readings_idx_error_msg;
	rc = sqlite3_exec(output_db, create_readings_idx_sql.c_str(), NULL, 0, & create_readings_idx_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating index READINGS_IDX: " << create_readings_idx_error_msg << endl;
		sqlite3_free(create_readings_idx_error_msg);
		exit(1);
	}
	//Then populate it using prepared statements within a single transaction:
	char * transaction_error_msg;
	sqlite3_exec(output_db, "BEGIN TRANSACTION", NULL, NULL, & transaction_error_msg);
	sqlite3_stmt * insert_into_readings_stmt;
	rc = sqlite3_prepare(output_db, "INSERT INTO READINGS VALUES (?,?,?)", -1, & insert_into_readings_stmt, 0);
	if (rc != SQLITE_OK) {
		cerr << "Error preparing statement." << endl;
		exit(1);
	}
	int row_id = 0;
	for (variation_unit vu : app.get_variation_units()) {
		string vu_id = vu.get_id();
		list<string> readings = vu.get_readings();
		for (string rdg : readings) {
			//Then insert a row containing these values:
			sqlite3_bind_int(insert_into_readings_stmt, 1, row_id);
			sqlite3_bind_text(insert_into_readings_stmt, 2, vu_id.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text(insert_into_readings_stmt, 3, rdg.c_str(), -1, SQLITE_STATIC);
			rc = sqlite3_step(insert_into_readings_stmt);
			if (rc != SQLITE_DONE) {
				cerr << "Error executing prepared statement." << endl;
				exit(1);
			}
			//Then reset the prepared statement so we can bind the next values to it:
			sqlite3_reset(insert_into_readings_stmt);
			row_id++;
		}
	}
	sqlite3_finalize(insert_into_readings_stmt);
	sqlite3_exec(output_db, "END TRANSACTION", NULL, NULL, & transaction_error_msg);
	return;
}

/**
 * Creates, indexes, and populates the READING_RELATIONS table.
 */
void populate_reading_relations_table(sqlite3 * output_db, const apparatus & app) {
	int rc; //to store SQLite macros
	//Create the READING_RELATIONS table:
	string create_reading_relations_sql = "DROP TABLE IF EXISTS READING_RELATIONS;"
			"CREATE TABLE READING_RELATIONS ("
			"ROW_ID INT NOT NULL, "
			"VARIATION_UNIT TEXT NOT NULL, "
			"PRIOR TEXT NOT NULL, "
			"POSTERIOR TEXT NOT NULL, "
			"WEIGHT REAL NOT NULL);";
	char * create_reading_relations_error_msg;
	rc = sqlite3_exec(output_db, create_reading_relations_sql.c_str(), NULL, 0, & create_reading_relations_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating table READING_RELATIONS: " << create_reading_relations_error_msg << endl;
		sqlite3_free(create_reading_relations_error_msg);
		exit(1);
	}
	//Denormalize it:
	string create_reading_relations_idx_sql = "DROP INDEX IF EXISTS READING_RELATIONS_IDX;"
			"CREATE INDEX READING_RELATIONS_IDX ON READING_RELATIONS (VARIATION_UNIT, PRIOR, POSTERIOR);";
	char * create_reading_relations_idx_error_msg;
	rc = sqlite3_exec(output_db, create_reading_relations_idx_sql.c_str(), NULL, 0, & create_reading_relations_idx_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating index READING_RELATIONS_IDX: " << create_reading_relations_idx_error_msg << endl;
		sqlite3_free(create_reading_relations_idx_error_msg);
		exit(1);
	}
	//Then populate it using prepared statements within a single transaction:
	char * transaction_error_msg;
	sqlite3_exec(output_db, "BEGIN TRANSACTION", NULL, NULL, & transaction_error_msg);
	sqlite3_stmt * insert_into_reading_relations_stmt;
	rc = sqlite3_prepare(output_db, "INSERT INTO READING_RELATIONS VALUES (?,?,?,?,?)", -1, & insert_into_reading_relations_stmt, 0);
	if (rc != SQLITE_OK) {
		cerr << "Error preparing statement." << endl;
		exit(1);
	}
	int row_id = 0;
	for (variation_unit vu : app.get_variation_units()) {
		string vu_id = vu.get_id();
		local_stemma ls = vu.get_local_stemma();
		list<local_stemma_edge> edges = ls.get_edges();
		for (local_stemma_edge e : edges) {
			//Then insert a row containing these values:
			sqlite3_bind_int(insert_into_reading_relations_stmt, 1, row_id);
			sqlite3_bind_text(insert_into_reading_relations_stmt, 2, vu_id.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text(insert_into_reading_relations_stmt, 3, e.prior.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text(insert_into_reading_relations_stmt, 4, e.posterior.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_double(insert_into_reading_relations_stmt, 5, e.weight);
			rc = sqlite3_step(insert_into_reading_relations_stmt);
			if (rc != SQLITE_DONE) {
				cerr << "Error executing prepared statement." << endl;
				exit(1);
			}
			//Then reset the prepared statement so we can bind the next values to it:
			sqlite3_reset(insert_into_reading_relations_stmt);
			row_id++;
		}
	}
	sqlite3_finalize(insert_into_reading_relations_stmt);
	sqlite3_exec(output_db, "END TRANSACTION", NULL, NULL, & transaction_error_msg);
	return;
}

/**
 * Creates, indexes, and populates the READING_SUPPORT table.
 * Rows will be populated in order of variation unit, then witness ID, 
 * following the order of witness IDs in the apparatus's list_wit member.
 */
void populate_reading_support_table(sqlite3 * output_db, const apparatus & app) {
	int rc; //to store SQLite macros
	//Create the READING_SUPPORT table:
	string create_reading_support_sql = "DROP TABLE IF EXISTS READING_SUPPORT;"
			"CREATE TABLE READING_SUPPORT ("
			"ROW_ID INT NOT NULL, "
			"VARIATION_UNIT TEXT NOT NULL, "
			"WITNESS TEXT NOT NULL, "
			"READING TEXT NOT NULL);";
	char * create_reading_support_error_msg;
	rc = sqlite3_exec(output_db, create_reading_support_sql.c_str(), NULL, 0, & create_reading_support_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating table READING_SUPPORT: " << create_reading_support_error_msg << endl;
		sqlite3_free(create_reading_support_error_msg);
		exit(1);
	}
	//Denormalize it:
	string create_reading_support_idx_sql = "DROP INDEX IF EXISTS READING_SUPPORT_IDX;"
			"CREATE INDEX READING_SUPPORT_IDX ON READING_SUPPORT (VARIATION_UNIT, WITNESS, READING);";
	char * create_reading_support_idx_error_msg;
	rc = sqlite3_exec(output_db, create_reading_support_idx_sql.c_str(), NULL, 0, & create_reading_support_idx_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating index READING_SUPPORT_IDX: " << create_reading_support_idx_error_msg << endl;
		sqlite3_free(create_reading_support_idx_error_msg);
		exit(1);
	}
	//Then populate it using prepared statements within a single transaction:
	char * transaction_error_msg;
	sqlite3_exec(output_db, "BEGIN TRANSACTION", NULL, NULL, & transaction_error_msg);
	sqlite3_stmt * insert_into_reading_support_stmt;
	rc = sqlite3_prepare(output_db, "INSERT INTO READING_SUPPORT VALUES (?,?,?,?)", -1, & insert_into_reading_support_stmt, 0);
	if (rc != SQLITE_OK) {
		cerr << "Error preparing statement." << endl;
		exit(1);
	}
	int row_id = 0;
	for (variation_unit vu : app.get_variation_units()) {
		string vu_id = vu.get_id();
		unordered_map<string, string> reading_support = vu.get_reading_support();
		for (string wit_id : app.get_list_wit()) {
			//Skip any witnesses that are lacunose here:
			if (reading_support.find(wit_id) == reading_support.end()) {
				continue;
			}
			string wit_rdg = reading_support.at(wit_id);
			//Then insert a row containing these values:
			sqlite3_bind_int(insert_into_reading_support_stmt, 1, row_id);
			sqlite3_bind_text(insert_into_reading_support_stmt, 2, vu_id.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text(insert_into_reading_support_stmt, 3, wit_id.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text(insert_into_reading_support_stmt, 4, wit_rdg.c_str(), -1, SQLITE_STATIC);
			rc = sqlite3_step(insert_into_reading_support_stmt);
			if (rc != SQLITE_DONE) {
				cerr << "Error executing prepared statement." << endl;
				exit(1);
			}
			//Then reset the prepared statement so we can bind the next values to it:
			sqlite3_reset(insert_into_reading_support_stmt);
			row_id++;
		}
	}
	sqlite3_finalize(insert_into_reading_support_stmt);
	sqlite3_exec(output_db, "END TRANSACTION", NULL, NULL, & transaction_error_msg);
	return;
}

/**
 * Creates, indexes, and populates the VARIATION_UNITS table.
 */
void populate_variation_units_table(sqlite3 * output_db, const apparatus & app) {
	int rc; //to store SQLite macros
	//Create the VARIATION_UNITS table:
	string create_variation_units_sql = "DROP TABLE IF EXISTS VARIATION_UNITS;"
			"CREATE TABLE VARIATION_UNITS ("
			"ROW_ID INT NOT NULL, "
			"VARIATION_UNIT TEXT NOT NULL, "
			"LABEL TEXT, "
			"CONNECTIVITY INT NOT NULL);";
	char * create_variation_units_error_msg;
	rc = sqlite3_exec(output_db, create_variation_units_sql.c_str(), NULL, 0, & create_variation_units_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating table VARIATION_UNITS: " << create_variation_units_error_msg << endl;
		sqlite3_free(create_variation_units_error_msg);
		exit(1);
	}
	//Denormalize it:
	string create_variation_units_idx_sql = "DROP INDEX IF EXISTS VARIATION_UNITS_IDX;"
			"CREATE INDEX VARIATION_UNITS_IDX ON VARIATION_UNITS (VARIATION_UNIT);";
	char * create_variation_units_idx_error_msg;
	rc = sqlite3_exec(output_db, create_variation_units_idx_sql.c_str(), NULL, 0, & create_variation_units_idx_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating index VARIATION_UNITS_IDX: " << create_variation_units_idx_error_msg << endl;
		sqlite3_free(create_variation_units_idx_error_msg);
		exit(1);
	}
	//Then populate it using prepared statements within a single transaction:
	char * transaction_error_msg;
	sqlite3_exec(output_db, "BEGIN TRANSACTION", NULL, NULL, & transaction_error_msg);
	sqlite3_stmt * insert_into_variation_units_stmt;
	rc = sqlite3_prepare(output_db, "INSERT INTO VARIATION_UNITS VALUES (?,?,?,?)", -1, & insert_into_variation_units_stmt, 0);
	if (rc != SQLITE_OK) {
		cerr << "Error preparing statement." << endl;
		exit(1);
	}
	int row_id = 0;
	for (variation_unit vu : app.get_variation_units()) {
		string id = vu.get_id();
		string label = vu.get_label();
		int connectivity = vu.get_connectivity();
		//Then insert a row containing these values:
		sqlite3_bind_int(insert_into_variation_units_stmt, 1, row_id);
		sqlite3_bind_text(insert_into_variation_units_stmt, 2, id.c_str(), -1, SQLITE_STATIC);
		sqlite3_bind_text(insert_into_variation_units_stmt, 3, label.c_str(), -1, SQLITE_STATIC);
		sqlite3_bind_int(insert_into_variation_units_stmt, 4, connectivity);
		rc = sqlite3_step(insert_into_variation_units_stmt);
		if (rc != SQLITE_DONE) {
			cerr << "Error executing prepared statement." << endl;
			exit(1);
		}
		//Then reset the prepared statement so we can bind the next values to it:
		sqlite3_reset(insert_into_variation_units_stmt);
		row_id++;
	}
	sqlite3_finalize(insert_into_variation_units_stmt);
	sqlite3_exec(output_db, "END TRANSACTION", NULL, NULL, & transaction_error_msg);
	return;
}

/**
 * Creates, indexes, and populates the GENEALOGICAL_COMPARISONS table.
 */
void populate_genealogical_comparisons_table(sqlite3 * output_db, const list<witness> & witnesses) {
	int rc; //to store SQLite macros
	//Create the GENEALOGICAL_COMPARISONS table:
	string create_genealogical_comparisons_sql = "DROP TABLE IF EXISTS GENEALOGICAL_COMPARISONS;"
			"CREATE TABLE GENEALOGICAL_COMPARISONS ("
			"ROW_ID INT NOT NULL, "
			"PRIMARY_WIT TEXT NOT NULL, "
			"SECONDARY_WIT TEXT NOT NULL, "
			"EXTANT BLOB NOT NULL, "
			"AGREEMENTS BLOB NOT NULL, "
			"PRIOR BLOB NOT NULL, "
			"POSTERIOR BLOB NOT NULL, "
			"NOREL BLOB NOT NULL, "
			"UNCLEAR BLOB NOT NULL, "
			"EXPLAINED BLOB NOT NULL, "
			"COST REAL NOT NULL);";
	char * create_genealogical_comparisons_error_msg;
	rc = sqlite3_exec(output_db, create_genealogical_comparisons_sql.c_str(), NULL, 0, & create_genealogical_comparisons_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating table GENEALOGICAL_COMPARISONS: " << create_genealogical_comparisons_error_msg << endl;
		sqlite3_free(create_genealogical_comparisons_error_msg);
		exit(1);
	}
	//Denormalize it:
	string create_genealogical_comparisons_idx_sql = "DROP INDEX IF EXISTS GENEALOGICAL_COMPARISONS_IDX;"
			"CREATE INDEX GENEALOGICAL_COMPARISONS_IDX ON GENEALOGICAL_COMPARISONS (PRIMARY_WIT, SECONDARY_WIT);";
	char * create_genealogical_comparisons_idx_error_msg;
	rc = sqlite3_exec(output_db, create_genealogical_comparisons_idx_sql.c_str(), NULL, 0, & create_genealogical_comparisons_idx_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating index GENEALOGICAL_COMPARISONS_IDX: " << create_genealogical_comparisons_idx_error_msg << endl;
		sqlite3_free(create_genealogical_comparisons_idx_error_msg);
		exit(1);
	}
	//Then populate it using prepared statements within a single transaction:
	char * transaction_error_msg;
	sqlite3_exec(output_db, "BEGIN TRANSACTION", NULL, NULL, & transaction_error_msg);
	sqlite3_stmt * insert_into_genealogical_comparisons_stmt;
	rc = sqlite3_prepare(output_db, "INSERT INTO GENEALOGICAL_COMPARISONS VALUES (?,?,?,?,?,?,?,?,?,?,?)", -1, & insert_into_genealogical_comparisons_stmt, 0);
	if (rc != SQLITE_OK) {
		cerr << "Error preparing statement." << endl;
		exit(1);
	}
	int row_id = 0;
	for (witness primary_wit : witnesses) {
		string primary_wit_id = primary_wit.get_id();
		for (witness secondary_wit : witnesses) {
			string secondary_wit_id = secondary_wit.get_id();
			genealogical_comparison comp = primary_wit.get_genealogical_comparison_for_witness(secondary_wit_id);
			//Serialize the bitmaps into byte arrays:
			Roaring extant = comp.extant;
			int extant_expected_size = (int) extant.getSizeInBytes();
			char * extant_buf = new char [extant_expected_size];
			extant.write(extant_buf);
			Roaring agreements = comp.agreements;
			int agreements_expected_size = (int) agreements.getSizeInBytes();
			char * agreements_buf = new char [agreements_expected_size];
			agreements.write(agreements_buf);
			Roaring prior = comp.prior;
			int prior_expected_size = (int) prior.getSizeInBytes();
			char * prior_buf = new char [prior_expected_size];
			prior.write(prior_buf);
			Roaring posterior = comp.posterior;
			int posterior_expected_size = (int) posterior.getSizeInBytes();
			char * posterior_buf = new char [posterior_expected_size];
			posterior.write(posterior_buf);
			Roaring norel = comp.norel;
			int norel_expected_size = (int) norel.getSizeInBytes();
			char * norel_buf = new char [norel_expected_size];
			norel.write(norel_buf);
			Roaring unclear = comp.unclear;
			int unclear_expected_size = (int) unclear.getSizeInBytes();
			char * unclear_buf = new char [unclear_expected_size];
			unclear.write(unclear_buf);
			Roaring explained = comp.explained;
			int explained_expected_size = (int) explained.getSizeInBytes();
			char * explained_buf = new char [explained_expected_size];
			explained.write(explained_buf);
			//Get the genealogical cost:
			float cost = comp.cost;
			//Then insert a row containing these values:
			sqlite3_bind_int(insert_into_genealogical_comparisons_stmt, 1, row_id);
			sqlite3_bind_text(insert_into_genealogical_comparisons_stmt, 2, primary_wit_id.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_text(insert_into_genealogical_comparisons_stmt, 3, secondary_wit_id.c_str(), -1, SQLITE_STATIC);
			sqlite3_bind_blob(insert_into_genealogical_comparisons_stmt, 4, extant_buf, extant_expected_size, SQLITE_STATIC);
			sqlite3_bind_blob(insert_into_genealogical_comparisons_stmt, 5, agreements_buf, agreements_expected_size, SQLITE_STATIC);
			sqlite3_bind_blob(insert_into_genealogical_comparisons_stmt, 6, prior_buf, prior_expected_size, SQLITE_STATIC);
			sqlite3_bind_blob(insert_into_genealogical_comparisons_stmt, 7, posterior_buf, posterior_expected_size, SQLITE_STATIC);
			sqlite3_bind_blob(insert_into_genealogical_comparisons_stmt, 8, norel_buf, norel_expected_size, SQLITE_STATIC);
			sqlite3_bind_blob(insert_into_genealogical_comparisons_stmt, 9, unclear_buf, unclear_expected_size, SQLITE_STATIC);
			sqlite3_bind_blob(insert_into_genealogical_comparisons_stmt, 10, explained_buf, explained_expected_size, SQLITE_STATIC);
			sqlite3_bind_double(insert_into_genealogical_comparisons_stmt, 11, cost);
			rc = sqlite3_step(insert_into_genealogical_comparisons_stmt);
			if (rc != SQLITE_DONE) {
				cerr << "Error executing prepared statement." << endl;
				delete[] extant_buf;
				delete[] agreements_buf;
				delete[] prior_buf;
				delete[] posterior_buf;
				delete[] norel_buf;
				delete[] unclear_buf;
				delete[] explained_buf;
				exit(1);
			}
			//Then clean up allocated memory and reset the prepared statement so we can bind the next values to it:
			delete[] extant_buf;
			delete[] agreements_buf;
			delete[] prior_buf;
			delete[] posterior_buf;
			delete[] norel_buf;
			delete[] unclear_buf;
			delete[] explained_buf;
			sqlite3_reset(insert_into_genealogical_comparisons_stmt);
			row_id++;
		}
	}
	sqlite3_finalize(insert_into_genealogical_comparisons_stmt);
	sqlite3_exec(output_db, "END TRANSACTION", NULL, NULL, & transaction_error_msg);
	return;
}

/**
 * Creates, indexes, and populates the WITNESSES table.
 */
void populate_witnesses_table(sqlite3 * output_db, const list<string> & list_wit) {
	int rc; //to store SQLite macros
	//Create the WITNESSES table:
	string create_witnesses_sql = "DROP TABLE IF EXISTS WITNESSES;"
			"CREATE TABLE WITNESSES ("
			"ROW_ID INT NOT NULL, "
			"WITNESS TEXT NOT NULL);";
	char * create_witnesses_error_msg;
	rc = sqlite3_exec(output_db, create_witnesses_sql.c_str(), NULL, 0, & create_witnesses_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating table WITNESSES: " << create_witnesses_error_msg << endl;
		sqlite3_free(create_witnesses_error_msg);
		exit(1);
	}
	//Denormalize it:
	string create_witnesses_idx_sql = "DROP INDEX IF EXISTS WITNESSES_IDX;"
			"CREATE INDEX WITNESSES_IDX ON WITNESSES (WITNESS);";
	char * create_witnesses_idx_error_msg;
	rc = sqlite3_exec(output_db, create_witnesses_idx_sql.c_str(), NULL, 0, & create_witnesses_idx_error_msg);
	if (rc != SQLITE_OK) {
		cerr << "Error creating index WITNESSES_IDX: " << create_witnesses_idx_error_msg << endl;
		sqlite3_free(create_witnesses_idx_error_msg);
		exit(1);
	}
	//Then populate it using prepared statements within a single transaction:
	char * transaction_error_msg;
	sqlite3_exec(output_db, "BEGIN TRANSACTION", NULL, NULL, & transaction_error_msg);
	sqlite3_stmt * insert_into_witnesses_stmt;
	rc = sqlite3_prepare(output_db, "INSERT INTO WITNESSES VALUES (?,?)", -1, & insert_into_witnesses_stmt, 0);
	if (rc != SQLITE_OK) {
		cerr << "Error preparing statement." << endl;
		exit(1);
	}
	int row_id = 0;
	for (string wit_id : list_wit) {
		//Then insert a row containing these values:
		sqlite3_bind_int(insert_into_witnesses_stmt, 1, row_id);
		sqlite3_bind_text(insert_into_witnesses_stmt, 2, wit_id.c_str(), -1, SQLITE_STATIC);
		rc = sqlite3_step(insert_into_witnesses_stmt);
		if (rc != SQLITE_DONE) {
			cerr << "Error executing prepared statement." << endl;
			exit(1);
		}
		//Then reset the prepared statement so we can bind the next values to it:
		sqlite3_reset(insert_into_witnesses_stmt);
		row_id++;
	}
	sqlite3_finalize(insert_into_witnesses_stmt);
	sqlite3_exec(output_db, "END TRANSACTION", NULL, NULL, & transaction_error_msg);
	return;
}

/**
 * Entry point to the script.
 */
int main(int argc, char* argv[]) {
	//Read in the command-line options:
	set<string> trivial_reading_types = set<string>();
	set<string> dropped_reading_types = set<string>();
	list<string> ignored_suffixes = list<string>();
	bool merge_splits = false;
	bool classic = false;
	int threshold = 0;
	string input_xml_name = string();
	string output_db_name = string();
	try {
		cxxopts::Options options("populate_db", "Parse the given collation XML file and populate the genealogical cache in the given SQLite database.");
		options.custom_help("[-h] [-t threshold] [-z trivial_reading_type_1 -z trivial_reading_type_2 ...] [-Z dropped_reading_type_1 -Z dropped_reading_type_2 ...] [-s ignored_suffix_1 -s ignored_suffix_2 ...] [--merge-splits] [--classic] input_xml output_db");
		options.positional_help("").show_positional_help();
		options.add_options("")
				("h,help", "print this help")
				("t,threshold", "minimum extant readings threshold", cxxopts::value<int>())
				("z", "reading type to treat as trivial (this may be used multiple times)", cxxopts::value<vector<string>>())
				("Z", "reading type to drop entirely (this may be used multiple times)", cxxopts::value<vector<string>>())
				("s", "ignored witness siglum suffixes (e.g., *, T, V, f) to drop entirely (this may be used multiple times)", cxxopts::value<vector<string>>())
				("merge-splits", "merge split attestations of the same reading", cxxopts::value<bool>())
				("classic", "calculate explained readings and costs using classic CBGM rules", cxxopts::value<bool>());
		options.add_options("positional")
				("input_xml", "collation file in TEI XML format", cxxopts::value<string>())
				("output_db", "output SQLite database (if an existing database is provided, its contents will be overwritten)", cxxopts::value<vector<string>>());
		options.parse_positional({"input_xml", "output_db"});
		auto args = options.parse(argc, argv);
		//Print help documentation and exit if specified:
		if (args.count("help")) {
			cout << options.help({"", "positional"}) << endl;
			exit(0);
		}
		//Parse the optional arguments:
		if (args.count("t")) {
			threshold = args["t"].as<int>();
		}
		if (args.count("z")) {
			for (string trivial_reading_type : args["z"].as<vector<string>>()) {
				trivial_reading_types.insert(trivial_reading_type);
			}
		}
		if (args.count("Z")) {
			for (string dropped_reading_type : args["Z"].as<vector<string>>()) {
				dropped_reading_types.insert(dropped_reading_type);
			}
		}
		if (args.count("s")) {
			for (string ignored_suffix : args["s"].as<vector<string>>()) {
				ignored_suffixes.push_back(ignored_suffix);
			}
		}
		if (args.count("merge-splits")) {
			merge_splits = args["merge-splits"].as<bool>();
		}
		if (args.count("classic")) {
			classic = args["classic"].as<bool>();
		}
		//Parse the positional arguments:
		if (!args.count("input_xml") || args.count("output_db") != 1) {
			cerr << "Error: 2 positional arguments (input_xml and output_db) are required." << endl;
			exit(1);
		}
		else {
			input_xml_name = args["input_xml"].as<string>();
			output_db_name = args["output_db"].as<vector<string>>()[0];
		}
	}
	catch (const cxxopts::OptionException & e) {
		cerr << "Error parsing options: " << e.what() << endl;
		exit(-1);
	}
	//Attempt to parse the input XML file as an apparatus:
	xml_document doc;
	xml_parse_result pr = doc.load_file(input_xml_name.c_str());
	if (!pr) {
		cerr << "Error: An error occurred while loading XML file " << input_xml_name << ": " << pr.description() << endl;
		exit(1);
	}
	xml_node tei_node = doc.child("TEI");
	if (!tei_node) {
		cerr << "Error: The XML file " << input_xml_name << " does not have a <TEI> element as its root element." << endl;
		exit(1);
	}
	apparatus app = apparatus(tei_node, merge_splits, trivial_reading_types, dropped_reading_types, ignored_suffixes);
	//If the user has specified a minimum extant readings threshold,
	//then repopulate the apparatus's witness list with just the IDs of witnesses that meet the threshold:
	if (threshold > 0) {
		cout << "Filtering out fragmentary witnesses... " << endl;
		list<string> list_wit = app.get_list_wit();
		list_wit.remove_if([&](const string & wit_id) {
			return app.get_extant_passages_for_witness(wit_id) < threshold;
		});
		app.set_list_wit(list_wit);
	}
	//Then initialize all of these witnesses:
	//TODO: Parallelize this step
	cout << "Initializing all witnesses (this may take a while)... " << endl;
	list<string> list_wit = app.get_list_wit();
	list<witness> witnesses = list<witness>();
	for (string wit_id : list_wit) {
		cout << "Calculating coherence for witness " << wit_id << "..." << endl;
		witness wit = witness(wit_id, app, classic);
		witnesses.push_back(wit);
	}
	//Now open the output database:
	cout << "Opening database..." << endl;
	sqlite3 * output_db;
	int rc = sqlite3_open(output_db_name.c_str(), & output_db);
	if (rc) {
		cerr << "Error opening database " << output_db_name << ": " << sqlite3_errmsg(output_db) << endl;
		exit(1);
	}
	//Populate each table:
	cout << "Populating table READINGS..." << endl;
	populate_readings_table(output_db, app);
	cout << "Populating table READING_RELATIONS..." << endl;
	populate_reading_relations_table(output_db, app);
	cout << "Populating table READING_SUPPORT..." << endl;
	populate_reading_support_table(output_db, app);
	cout << "Populating table VARIATION_UNITS..." << endl;
	populate_variation_units_table(output_db, app);
	cout << "Populating table GENEALOGICAL_COMPARISONS..." << endl;
	populate_genealogical_comparisons_table(output_db, witnesses);
	cout << "Populating table WITNESSES..." << endl;
	populate_witnesses_table(output_db, list_wit);
	//Finally, close the output database:
	cout << "Closing database..." << endl;
	sqlite3_close(output_db);
	cout << "Database closed." << endl;
	exit(0);
}
