#pragma once

//#include <cstdlib>
#include <cstdio>
#include <vector>
#include <string>
#include <string.h>
#include "sqlite/sqlite3.h"

static const char* createRef = "CREATE TABLE IF NOT EXISTS `reference` (`device_id` INT NOT NULL UNIQUE, `device_name` TEXT NOT NULL, `device_in_use` BIT(1) NOT NULL DEFAULT '0', `device_location` TEXT, `device_costplace` TEXT, `device_next_checkup` INT DEFAULT NULL, PRIMARY KEY (`device_id`));";
static const char* createDev = "CREATE TABLE IF NOT EXISTS `device` (`device_id` INT NOT NULL UNIQUE, `model` TEXT, `serial_number` INT, `supplier` TEXT, `manufacturer` TEXT, `purchase_date` INT, `warranty_date` INT, `department` TEXT, `costplace_name` TEXT, `administrator` TEXT, `replacement` TEXT, `has_log` BIT(1), `has_manual` BIT(1), `fitness_freq` INT, `internal_freq` INT, `last_internal_check` INT, `next_internal_check` INT, `external_company` TEXT, `external_freq` INT, `last_external_check` INT, `next_external_check` INT, `contract_desc` TEXT, `setup_date` INT, `decommission_date` INT, `wattage` FLOAT, PRIMARY KEY (`device_id`));";
static const char* createLog = "CREATE TABLE IF NOT EXISTS `log` (`date` INT NOT NULL, `logger` TEXT NOT NULL, `log` TEXT NOT NULL, PRIMARY KEY (`date`));";

struct DeviceData {
	// reference (also in device)
	int id;
	std::string name;
	bool inUse;
	std::string location;
	std::string costplace;

	// device info
	std::string model;
	int serialnumber;
	std::string supplier;
	std::string manufacturer;
	int purchaseDate;
	int warrantyDate;
	std::string department;
	std::string costplaceName;

	// internal info
	std::string admin;
	std::string replacement;
	bool hasLog;
	bool hasManual;

	// internal check
	int useabilityFrequency;
	int internalFrequency;
	int lastInternalCheck;
	int nextInternalCheck;

	// external check
	std::string externalCompany;
	int externalFrequency;
	int lastExternalCheck;
	int nextExternalCheck;
	std::string contractDescription;

	// status
	int dateOfSetup;
	int dateOfDecommissioning;
	float wattage;

	// log
	std::vector<int> logDate; // DATE
	std::vector<std::string> logLogger;
	std::vector<std::string> logLog;
};

namespace DB {
	static bool execQuery(sqlite3*& con, const char* t) {
		char* err;
		int sc = 0;
		if ((sc = sqlite3_exec(con, t, nullptr, nullptr, &err)) != SQLITE_OK) { // gives error: misuse
			printf_s("[DB-QUERY-ERROR] %i\n", sc);
			printf_s("Error message: %s\n", err);// crashes
			return false;
		}
		return true;
	}

	static bool createConnection(sqlite3*& con, const char* src, const char* create) {
		sqlite3_open(src, &con);
		if (con == nullptr) {
			printf_s("[DB-ERROR] Failed to open database connection at \"%s\"", src);
			return false;
		}

		// make sure database exists
		return execQuery(con, create);;
	}

	static void addDevice(DeviceData& data) {
		sqlite3* connection;

		// setup data in reference
		if (createConnection(connection, "db/reference.db", createRef)) {
			// create query
			std::string t = "INSERT INTO `reference` (`device_id`, `device_name`, `device_in_use`, `device_location`, `device_costplace`) VALUES (";
			t += std::to_string(data.id);
			t += ", \"";
			t += data.name;
			t += "\", ";
			t += data.inUse ? "1" : "0";
			t += ", \"";
			t += data.location;
			t += "\", \"";
			t += data.costplace;
			t += "\");";

			// execute query
			execQuery(connection, t.c_str());

			// close connection
			sqlite3_close(connection);
			connection = nullptr;
		}

		// setup device info in its used costplace database
		std::string tmp = "db/costplaces/" + data.costplace + ".db";
		if (createConnection(connection, tmp.c_str(), createDev)) {
			// create query
			std::string t = "INSERT INTO `device` (device_id, model, serial_number, supplier, manufacturer, purchase_date, warranty_date, department, costplace_name, administrator, replacement, has_log, has_manual, fitness_freq, internal_freq, last_internal_check, next_internal_check, external_company, external_freq, last_external_check, next_external_check, contract_desc, setup_date, decommission_date, wattage) VALUES (";
			t += std::to_string(data.id);

			// device info
			t += ", \"";
			t += data.model;
			t += "\", ";
			t += std::to_string(data.serialnumber);
			t += ", \"";
			t += data.supplier;
			t += "\", \"";
			t += data.manufacturer;
			t += "\", ";
			t += std::to_string(data.purchaseDate);
			t += ", ";
			t += std::to_string(data.warrantyDate);
			t += ", \"";
			t += data.department;
			t += "\", \"";
			t += data.costplaceName;

			// internal info
			t += "\", \"";
			t += data.admin;
			t += "\", \"";
			t += data.replacement;
			t += "\", ";
			t += data.hasLog ? "1" : "0";
			t += ", ";
			t += data.hasManual ? "1" : "0";
			
			// internal check
			t += ", ";
			t += std::to_string(data.useabilityFrequency);
			t += ", ";
			t += std::to_string(data.internalFrequency);
			t += ", ";
			t += std::to_string(data.lastInternalCheck); // date
			t += ", ";
			t += std::to_string(data.nextInternalCheck); // date

			// external check
			t += ", \"";
			t += data.externalCompany;
			t += "\", ";
			t += std::to_string(data.externalFrequency);
			t += ", ";
			t += std::to_string(data.lastExternalCheck); // date
			t += ", ";
			t += std::to_string(data.nextExternalCheck); // date
			t += ", \"";
			t += data.contractDescription;

			// status
			t += "\", ";
			t += std::to_string(data.dateOfSetup); // date/string?
			t += ", ";
			t += std::to_string(data.dateOfDecommissioning); // date/string?
			t += ", ";
			t += std::to_string(data.wattage);
			t += ");";

			// execute query
			execQuery(connection, t.c_str());

			// close connection
			sqlite3_close(connection);
			connection = nullptr;
		}

		// setup a log database entry file
		tmp = "db/logs/" + std::to_string(data.id) + ".db";
		if (createConnection(connection, tmp.c_str(), createLog)) { // TODO
			// create query
			std::string t = "INSERT INTO `log` (`date`, `logger`, `log`) VALUES (0, \"ABS\", \"Created device entry\")";

			// execute query
			execQuery(connection, t.c_str());

			// close connection
			sqlite3_close(connection);
			connection = nullptr;
		}
	}
}

struct FilterMenu {
private:
	// DB data
	const char* src = "db/reference.db";
	sqlite3* connection = nullptr;

public:
	// data buffers
	std::vector<int> id;
	std::vector<std::string> name;
	std::vector<bool> inUse;
	std::vector<std::string> location;
	std::vector<std::string> costplace;
	std::vector<int> nextCheckup;

	// functions
	FilterMenu() {}
	~FilterMenu() { clearBuffers(); }
	void reload() {
		// clear data buffers
		clearBuffers();

		if (DB::createConnection(connection, src, createRef)) {
			// get data from database if there is any
			sqlite3_stmt* stmt;
			sqlite3_prepare_v2(connection, "SELECT device_id, device_name, device_in_use, device_location, device_costplace, device_next_checkup FROM reference ORDER BY device_id ASC", -1, &stmt, NULL);
			while (sqlite3_step(stmt) != SQLITE_DONE) {
				id.push_back(sqlite3_column_int(stmt, 0));
				name.push_back(std::string((const char*)sqlite3_column_text(stmt, 1)));
				inUse.push_back(sqlite3_column_bytes(stmt, 2));
				location.push_back(std::string((const char*)sqlite3_column_text(stmt, 3)));
				costplace.push_back(std::string((const char*)sqlite3_column_text(stmt, 4))); // unsigned char* from sqlite get their data removed when exiting this function because of the deallocator which makes displaying them impossible
				nextCheckup.push_back(0); // TODO: checkup
			}
			sqlite3_finalize(stmt);

			// close connection
			sqlite3_close(connection);
			connection = nullptr;
		}
	}

private:
	void clearBuffers() {
		id.clear();
		name.clear();
		inUse.clear();
		location.clear();
		costplace.clear();
		nextCheckup.clear();
	}
};

struct FilterMenu;

struct DeviceMenu {
private:
	const char* src = "db/reference.db";
	sqlite3* connection = nullptr;
public:

	DeviceMenu() {}
	~DeviceMenu() { clearBuffers(); }

	// Data
	bool isLoaded = false;
	DeviceData data;

	// public functions
	bool loadDevice(int id, FilterMenu& filter) {
		// clear data buffers
		unloadDevice();

		// get info from filtermenu
		for (int i = 0; i < filter.id.size(); i++) {
			if (filter.id[i] == id) {
				data.name = filter.name[i];
				data.inUse = filter.inUse[i];
				data.location = filter.location[i];
				data.costplace = filter.costplace[i];
				break;
			}
		}

		// read data if it can be found into DeviceData
		std::string tmp = "db/costplaces/" + data.costplace + ".db";
		if (DB::createConnection(connection, tmp.c_str(), createDev)) {
			// get data from database if there is any
			sqlite3_stmt* stmt;
			tmp = "SELECT device_id, model, serial_number, supplier, manufacturer, purchase_date, warranty_date, department, costplace_name, administrator, replacement, has_log, has_manual, fitness_freq, internal_freq, last_internal_check, next_internal_check, external_company, external_freq, last_external_check, next_external_check, contract_desc, setup_date, decommission_date, wattage FROM `device` WHERE device_id = " + std::to_string(id) + " LIMIT 1";
			sqlite3_prepare_v2(connection, tmp.c_str() , -1, &stmt, NULL);
			int sc; // get sql code
			while ((sc = sqlite3_step(stmt)) != SQLITE_DONE) {
				data.id = sqlite3_column_int(stmt, 0);
				// other device data is gained from reference list
				if (sc == SQLITE_MISUSE) {
					printf_s("Something went wrong while trying to load the device data for device_id=%i, error code for SQLITE_MISUSE (21) has been returned to the program...\n", id);
					unloadDevice();
					sqlite3_finalize(stmt);
					sqlite3_close(connection);
					return false;
				}

				// device info
				data.model = std::string((const char*)sqlite3_column_text(stmt, 1));
				data.serialnumber = sqlite3_column_int(stmt, 2);
				data.supplier = std::string((const char*)sqlite3_column_text(stmt, 3));
				data.manufacturer = std::string((const char*)sqlite3_column_text(stmt, 4));
				data.purchaseDate = sqlite3_column_int(stmt, 5); // date
				data.warrantyDate = sqlite3_column_int(stmt, 6); // date
				data.department = std::string((const char*)sqlite3_column_text(stmt, 7));
				data.costplaceName = std::string((const char*)sqlite3_column_text(stmt, 8));

				// internal info
				data.admin = std::string((const char*)sqlite3_column_text(stmt, 9));
				data.replacement = std::string((const char*)sqlite3_column_text(stmt, 10));
				data.hasLog = sqlite3_column_bytes(stmt, 11) != 0;
				data.hasManual = sqlite3_column_bytes(stmt, 12) != 0;

				// internal check
				data.useabilityFrequency = sqlite3_column_int(stmt, 13);
				data.internalFrequency = sqlite3_column_int(stmt, 14);
				data.lastInternalCheck = sqlite3_column_int(stmt, 15); // date
				data.nextInternalCheck = sqlite3_column_int(stmt, 16); // date

				// external check
				data.externalCompany = std::string((const char*)sqlite3_column_text(stmt, 17));
				data.externalFrequency = sqlite3_column_int(stmt, 18);
				data.lastExternalCheck = sqlite3_column_int(stmt, 19); // date
				data.nextExternalCheck = sqlite3_column_int(stmt, 20); // date
				data.contractDescription = std::string((const char*)sqlite3_column_text(stmt, 21));

				// status
				data.dateOfSetup = sqlite3_column_int(stmt, 22); // date/string?
				data.dateOfDecommissioning = sqlite3_column_int(stmt, 23); // date/string?
				data.wattage = sqlite3_column_double(stmt, 24);

				isLoaded = true;
			}
			sqlite3_finalize(stmt);

			// close connection
			sqlite3_close(connection);
			connection = nullptr;
			if (!isLoaded) { unloadDevice(); return false; }
		}
		// read data if it can be found into DeviceData
		tmp = "db/logs/" + std::to_string(data.id) + ".db";
		if (DB::createConnection(connection, tmp.c_str(), createDev)) {
			// get data from database if there is any
			sqlite3_stmt* stmt;
			tmp = "SELECT date, logger, log FROM `log`";
			sqlite3_prepare_v2(connection, tmp.c_str(), -1, &stmt, NULL);
			int sc; // get sql code
			while ((sc = sqlite3_step(stmt)) != SQLITE_DONE) {
				data.id = sqlite3_column_int(stmt, 0);
				// other device data is gained from reference list
				if (sc == SQLITE_MISUSE) {
					printf_s("Something went wrong while trying to load the device data for device_id=%i, error code for SQLITE_MISUSE (21) has been returned to the program...\n", id);
					unloadDevice();
					sqlite3_finalize(stmt);
					sqlite3_close(connection);
					return false;
				}

				// TODO: read out device log
				data.logDate.push_back(sqlite3_column_int(stmt, 0));
				data.logLogger.push_back(std::string((const char*)sqlite3_column_text(stmt, 1)));
				data.logLog.push_back(std::string((const char*)sqlite3_column_text(stmt, 2)));

				isLoaded = true;
			}
			sqlite3_finalize(stmt);

			// close connection
			sqlite3_close(connection);
			connection = nullptr;
			if (isLoaded) { return true; }
		}
		unloadDevice();
		return false;
	}

	void reload(FilterMenu& filter) {
		loadDevice(data.id, filter);
	}

	void unloadDevice() { clearBuffers(); }
private:
	void clearBuffers() {
		isLoaded = false;
		data.id = -1;
		data.name = "";
		data.inUse = false;
		data.location = "";
		data.costplace = "";

		// device info
		data.model = "";
		data.serialnumber = 0;
		data.supplier = "";
		data.manufacturer = "";
		data.purchaseDate = 0; // date
		data.warrantyDate = 0; // date
		data.department = "";
		data.costplaceName = "";

		// internal info
		data.admin = "";
		data.replacement = "";
		data.hasLog = false;
		data.hasManual = false;

		// internal check
		data.useabilityFrequency = 0;
		data.internalFrequency = 0;
		data.lastInternalCheck = 0; // date
		data.nextInternalCheck = 0; // date

		// external check
		data.externalCompany = "";
		data.externalFrequency = 0;
		data.lastExternalCheck = 0; // date
		data.nextExternalCheck = 0; // date
		data.contractDescription = "";

		// status
		data.dateOfSetup = 0; // date/string?
		data.dateOfDecommissioning = 0; // date/string?
		data.wattage = 0.0f;

		// log
		data.logDate.clear();
		data.logLogger.clear();
		data.logLog.clear();
	}
};