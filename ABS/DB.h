#pragma once

#include <cstdio> // printf_s()
#include <vector> // std::vector<T>
#include <string> // std::string
#include <string.h> // std::to_string()
#include <ctime> // time_t
#include "sqlite/sqlite3.h" // Database library

static const char* createRef = "CREATE TABLE IF NOT EXISTS `reference` (`device_id` INT NOT NULL UNIQUE, `device_name` TEXT NOT NULL, `device_in_use` BIT(1) NOT NULL DEFAULT '0', `device_location` TEXT, `device_costplace` TEXT, `device_next_checkup` BIGINT DEFAULT NULL, PRIMARY KEY (`device_id`));";
static const char* createDev = "CREATE TABLE IF NOT EXISTS `device` (`device_id` INT NOT NULL UNIQUE, `model` TEXT, `serial_number` INT, `supplier` TEXT, `manufacturer` TEXT, `purchase_date` BIGINT, `warranty_date` BIGINT, `department` TEXT, `costplace_name` TEXT, `administrator` TEXT, `replacement` TEXT, `has_log` BIT(1), `has_manual` BIT(1), `fitness_freq` INT, `internal_freq` INT, `last_internal_check` BIGINT, `next_internal_check` BIGINT, `external_company` TEXT, `external_freq` INT, `last_external_check` BIGINT, `next_external_check` BIGINT, `contract_desc` TEXT, `setup_date` BIGINT, `decommission_date` BIGINT, `wattage` FLOAT, PRIMARY KEY (`device_id`));";
static const char* createLog = "CREATE TABLE IF NOT EXISTS `log` (`date` BIGINT NOT NULL UNIQUE, `logger` TEXT NOT NULL, `log` TEXT NOT NULL, PRIMARY KEY (`date`));";
static const std::string unused = "outOfUse";
///////////
// Dates //
///////////

struct Date {
private:
	tm* d = new tm;
public:
	time_t t = time(&t); // public: only use in frequencyDateOffset

	Date() { update(time(0)); };
	Date(time_t e) { update(e); };
	Date(int year, int month, int day) {
		d->tm_year = year - 1900;
		d->tm_mon = month - 1;
		d->tm_mday = day+1;
		d->tm_hour = 1;
		d->tm_min = 1;
		d->tm_sec = 1;
		t = mktime(d);
		update(t);
	}
	~Date() {};

private:
	void update(time_t e) {
		t = e;
		localtime_s(d, &t);
	}

public:
	bool operator< (Date& o) { return t <  o.t; }
	bool operator<=(Date& o) { return t <= o.t; }
	bool operator> (Date& o) { return t >  o.t; }
	bool operator>=(Date& o) { return t >= o.t; }
	bool operator==(Date& o) { return t == o.t; }
	bool operator!=(Date& o) { return t != o.t; }
	Date operator+ (Date& o)  { return Date(t + o.t); }
	Date operator- (Date& o)  { return Date(t - o.t); }
	Date operator+ (time_t o) { return Date(t + o  ); }
	Date operator- (time_t o) { return Date(t - o  ); }

	const std::string asString() const { return std::to_string(d->tm_year + 1900) + "-" + std::to_string(d->tm_mon + 1) + "-" + std::to_string(d->tm_mday); }
	const uint64_t asInt64() { return t; }

	static const Date getOffset(int y, int m, int d) { return Date(1970 + y, 1 + m, 1 + d).t - Date(1970, 1, 1).t; }
	static void filterDateInput(std::string str) {
		std::string tmp;
		for (int i = 0; i < str.size(); i++) {
			switch (str[i]) {
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
				case '-':
					tmp += str[i]; 
					break;
				default: break;
			}
		}
		str = tmp;
	}
};

//////////////////
// Data structs //
//////////////////

struct DeviceData {
	// Reference (also in device)
	int id;
	std::string name;
	bool inUse;
	std::string location;
	std::string costplace;

	// Device info
	std::string model;
	int serialnumber;
	std::string supplier;
	std::string manufacturer;
	Date purchaseDate;
	Date warrantyDate;
	std::string department;
	std::string costplaceName;

	// Internal info
	std::string admin;
	std::string replacement;
	bool hasLog;
	bool hasManual;

	// Internal check
	int useabilityFrequency;
	int internalFrequency;
	Date lastInternalCheck;
	Date nextInternalCheck;

	// External check
	std::string externalCompany;
	int externalFrequency;
	Date lastExternalCheck;
	Date nextExternalCheck;
	std::string contractDescription;

	// Status
	Date dateOfSetup;
	Date dateOfDecommissioning;
	float wattage;

	// Log
	std::vector<Date> logDate;
	std::vector<std::string> logLogger;
	std::vector<std::string> logLog;
};

struct EditDeviceDates {
	// Device info
	std::string purchaseDateString;
	std::string warrantyDateString;

	// Internal check
	std::string lastInternalCheckString;
	std::string nextInternalCheckString;

	// External check
	std::string lastExternalCheckString;
	std::string nextExternalCheckString;

	// Status
	std::string dateOfSetupString;
	std::string dateOfDecommissioningString;
};

//////////////
// Database //
//////////////

namespace DB {
	static const bool execQuery(sqlite3*& con, const char* t) {
		int sc = 0;
		if ((sc = sqlite3_exec(con, t, nullptr, nullptr, nullptr)) != SQLITE_OK) { printf_s("[DB-QUERY-ERROR] %i\n", sc); return false; }
		return true;
	}

	static bool createConnection(sqlite3*& con, const char* src, const char* create) {
		sqlite3_open(src, &con);
		if (con == nullptr) {
			printf_s("[DB-ERROR] Failed to open database connection at \"%s\"", src);
			return false;
		}

		// Make sure database exists
		return execQuery(con, create);
	}

	static void addDevice(const DeviceData& data) {
		sqlite3* connection;

		// Setup data in reference
		if (createConnection(connection, "db/reference.db", createRef)) {
			// Create query
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

			// Execute query
			execQuery(connection, t.c_str());

			// Close connection
			sqlite3_close(connection);
			connection = nullptr;
		}

		// Setup device info in its used costplace database
		std::string tmp = "db/costplaces/" + (data.inUse ? data.costplace : unused) + ".db";
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
			t += data.purchaseDate.asString();
			t += ", ";
			t += data.warrantyDate.asString();
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
			t += data.lastInternalCheck.asString(); // date
			t += ", ";
			t += data.nextInternalCheck.asString(); // date

			// external check
			t += ", \"";
			t += data.externalCompany;
			t += "\", ";
			t += std::to_string(data.externalFrequency);
			t += ", ";
			t += data.lastExternalCheck.asString(); // date
			t += ", ";
			t += data.nextExternalCheck.asString(); // date
			t += ", \"";
			t += data.contractDescription;

			// status
			t += "\", ";
			t += data.dateOfSetup.asString(); // date/string?
			t += ", ";
			t += data.dateOfDecommissioning.asString(); // date/string?
			t += ", ";
			t += std::to_string(data.wattage);
			t += ");";

			// execute query
			execQuery(connection, t.c_str());

			// close connection
			sqlite3_close(connection);
			connection = nullptr;
		}

		// Setup a log database entry file
		tmp = "db/logs/" + std::to_string(data.id) + ".db";
		if (createConnection(connection, tmp.c_str(), createLog)) {
			// create query
			Date today;
			std::string t = "INSERT INTO `log` (`date`, `logger`, `log`) VALUES (" + today.asString() + ", \"ABS\", \"Created device entry\")";

			// execute query
			execQuery(connection, t.c_str());

			// close connection
			sqlite3_close(connection);
			connection = nullptr;
		}
	}

	static void addDeviceLog(int id, Date& date, std::string& logger, std::string log) {
		sqlite3* connection;
		std::string tmp = "db/logs/" + std::to_string(id) + ".db";
		if (createConnection(connection, tmp.c_str(), createLog)) {
			// create query
			std::string t = "INSERT INTO `log` (`date`, `logger`, `log`) VALUES (" + date.asString() + ", \"" + logger + "\", \"" + log + "\")";

			// execute query
			execQuery(connection, t.c_str());

			// close connection
			sqlite3_close(connection);
			connection = nullptr;
		}
	}

	static const void updateLog(const int id, const char* t) {
		sqlite3* connection;
		std::string tmp = "db/logs/" + std::to_string(id) + ".db";
		if (createConnection(connection, tmp.c_str(), createLog)) {
			// execute query
			execQuery(connection, t);

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
	// Data buffers
	std::vector<int> id;
	std::vector<std::string> name;
	std::vector<bool> inUse;
	std::vector<std::string> location;
	std::vector<std::string> costplace;
	std::vector<Date> nextCheckup;

	// Functions
	FilterMenu() {}
	~FilterMenu() { clearBuffers(); }
	void reload() {
		// Clear data buffers
		clearBuffers();

		if (DB::createConnection(connection, src, createRef)) {
			// Get data from database if there is any
			sqlite3_stmt* stmt;
			sqlite3_prepare_v2(connection, "SELECT device_id, device_name, device_in_use, device_location, device_costplace, device_next_checkup FROM reference ORDER BY device_id ASC", -1, &stmt, NULL);
			while (sqlite3_step(stmt) != SQLITE_DONE) {
				id.push_back(sqlite3_column_int(stmt, 0));
				name.push_back(std::string((const char*)sqlite3_column_text(stmt, 1)));
				int i = sqlite3_column_bytes(stmt, 2);
				printf_s("ID%i: %i\n", id.at(id.size()-1), i);
				inUse.push_back(i);
				location.push_back(std::string((const char*)sqlite3_column_text(stmt, 3)));
				costplace.push_back(std::string((const char*)sqlite3_column_text(stmt, 4)));
				nextCheckup.push_back(Date(sqlite3_column_int64(stmt, 5)));
			}
			sqlite3_finalize(stmt);

			// Close connection
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

struct DeviceMenu {
private:
	const char* src = "db/reference.db";
	sqlite3* connection = nullptr;
public:

	DeviceMenu() {}
	~DeviceMenu() { clearBuffers(); }

	bool isLoaded = false;
	DeviceData data;

	const bool loadDevice(int id, const FilterMenu& filter) {
		// Clear data buffers
		unloadDevice();

		// Get info from filtermenu
		for (int i = 0; i < filter.id.size(); i++) {
			if (filter.id[i] == id) {
				data.name = filter.name[i];
				data.inUse = filter.inUse[i];
				data.location = filter.location[i];
				data.costplace = filter.costplace[i];
				break;
			}
		}

		// Read data if it can be found into DeviceData
		std::string tmp = "db/costplaces/" + (data.inUse ? data.costplace : unused) + ".db";
		if (DB::createConnection(connection, tmp.c_str(), createDev)) {
			// Get data from database if there is any
			sqlite3_stmt* stmt;
			tmp = "SELECT device_id, model, serial_number, supplier, manufacturer, purchase_date, warranty_date, department, costplace_name, administrator, replacement, has_log, has_manual, fitness_freq, internal_freq, last_internal_check, next_internal_check, external_company, external_freq, last_external_check, next_external_check, contract_desc, setup_date, decommission_date, wattage FROM `device` WHERE device_id = " + std::to_string(id) + " LIMIT 1";
			sqlite3_prepare_v2(connection, tmp.c_str() , -1, &stmt, NULL);
			int sc; // Get sql code
			while ((sc = sqlite3_step(stmt)) != SQLITE_DONE) {
				
				// Other device data is gained from reference list
				if (sc == SQLITE_MISUSE) {
					printf_s("Something went wrong while trying to load the device data for device_id=%i, error code for SQLITE_MISUSE (21) has been returned to the program...\n", id);
					unloadDevice();
					sqlite3_finalize(stmt);
					sqlite3_close(connection);
					return false;
				}

				// Device info
				data.id = sqlite3_column_int(stmt, 0);
				data.model = std::string((const char*)sqlite3_column_text(stmt, 1));
				data.serialnumber = sqlite3_column_int(stmt, 2);
				data.supplier = std::string((const char*)sqlite3_column_text(stmt, 3));
				data.manufacturer = std::string((const char*)sqlite3_column_text(stmt, 4));
				data.purchaseDate = Date(sqlite3_column_int64(stmt, 5));
				data.warrantyDate = Date(sqlite3_column_int64(stmt, 6));
				data.department = std::string((const char*)sqlite3_column_text(stmt, 7));
				data.costplaceName = std::string((const char*)sqlite3_column_text(stmt, 8));

				// Internal info
				data.admin = std::string((const char*)sqlite3_column_text(stmt, 9));
				data.replacement = std::string((const char*)sqlite3_column_text(stmt, 10));
				data.hasLog = sqlite3_column_bytes(stmt, 11) != 0;
				data.hasManual = sqlite3_column_bytes(stmt, 12) != 0;

				// Internal check
				data.useabilityFrequency = sqlite3_column_int(stmt, 13);
				data.internalFrequency = sqlite3_column_int(stmt, 14);
				data.lastInternalCheck = Date(sqlite3_column_int64(stmt, 15));
				data.nextInternalCheck = Date(sqlite3_column_int64(stmt, 16));

				// External check
				data.externalCompany = std::string((const char*)sqlite3_column_text(stmt, 17));
				data.externalFrequency = sqlite3_column_int(stmt, 18);
				data.lastExternalCheck = Date(sqlite3_column_int64(stmt, 19));
				data.nextExternalCheck = Date(sqlite3_column_int64(stmt, 20));
				data.contractDescription = std::string((const char*)sqlite3_column_text(stmt, 21));

				// Status
				data.dateOfSetup = Date(sqlite3_column_int64(stmt, 22));
				data.dateOfDecommissioning = Date(sqlite3_column_int64(stmt, 23));
				data.wattage = sqlite3_column_double(stmt, 24);

				isLoaded = true;
			}
			sqlite3_finalize(stmt);

			// Close connection
			sqlite3_close(connection);
			connection = nullptr;
			if (!isLoaded) { unloadDevice(); return false; }
		}

		// Read data from device logs into DeviceData
		tmp = "db/logs/" + std::to_string(data.id) + ".db";
		if (DB::createConnection(connection, tmp.c_str(), createDev)) {
			// Get data from database if there is any
			sqlite3_stmt* stmt;
			tmp = "SELECT date, logger, log FROM `log`";
			sqlite3_prepare_v2(connection, tmp.c_str(), -1, &stmt, NULL);
			int sc; // Get sql code
			while ((sc = sqlite3_step(stmt)) != SQLITE_DONE) {
				data.id = sqlite3_column_int(stmt, 0);
				// Other device data is gained from reference list
				if (sc == SQLITE_MISUSE) {
					printf_s("Something went wrong while trying to load the device data for device_id=%i, error code for SQLITE_MISUSE (21) has been returned to the program...\n", id);
					unloadDevice();
					sqlite3_finalize(stmt);
					sqlite3_close(connection);
					return false;
				}
				data.logDate.push_back(Date(sqlite3_column_int64(stmt, 0)));
				data.logLogger.push_back(std::string((const char*)sqlite3_column_text(stmt, 1)));
				data.logLog.push_back(std::string((const char*)sqlite3_column_text(stmt, 2)));

				isLoaded = true;
			}
			sqlite3_finalize(stmt);

			// Close connection
			sqlite3_close(connection);
			connection = nullptr;
			if (isLoaded) { return true; }
		}
		unloadDevice();
		return false;
	}

	void reload(FilterMenu& filter) { loadDevice(data.id, filter); }

	void unloadDevice() { clearBuffers(); }
private:
	void clearBuffers() {
		isLoaded = false;
		data.id = -1;
		data.name = "";
		data.inUse = false;
		data.location = "";
		data.costplace = "";

		// Device info
		data.model = "";
		data.serialnumber = 0;
		data.supplier = "";
		data.manufacturer = "";
		data.purchaseDate = Date();
		data.warrantyDate = Date();
		data.department = "";
		data.costplaceName = "";

		// Internal info
		data.admin = "";
		data.replacement = "";
		data.hasLog = false;
		data.hasManual = false;

		// Internal check
		data.useabilityFrequency = 0;
		data.internalFrequency = 0;
		data.lastInternalCheck = Date();
		data.nextInternalCheck = Date();

		// External check
		data.externalCompany = "";
		data.externalFrequency = 0;
		data.lastExternalCheck = Date();
		data.nextExternalCheck = Date();
		data.contractDescription = "";

		// Status
		data.dateOfSetup = Date();
		data.dateOfDecommissioning = Date();
		data.wattage = 0.0f;

		// Log
		data.logDate.clear();
		data.logLogger.clear();
		data.logLog.clear();
	}
};

namespace DB {
	static void moveDevice(DeviceMenu& old, DeviceMenu& edited) {
		sqlite3* connection;
		std::string tmp = "db/costplaces/" + (old.data.inUse ? old.data.costplace : unused) + ".db";
		if (createConnection(connection, tmp.c_str(), createDev)) {
			tmp = "DELETE FROM `device` WHERE device_id=" + std::to_string(old.data.id);
			DB::execQuery(connection, tmp.c_str());
		}

		DeviceData& data = edited.data;
		// Setup device info in its used costplace database
		tmp = "db/costplaces/" + (data.inUse ? data.costplace : unused) + ".db";
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
			t += data.purchaseDate.asString();
			t += ", ";
			t += data.warrantyDate.asString();
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
			t += data.lastInternalCheck.asString(); // date
			t += ", ";
			t += data.nextInternalCheck.asString(); // date

			// external check
			t += ", \"";
			t += data.externalCompany;
			t += "\", ";
			t += std::to_string(data.externalFrequency);
			t += ", ";
			t += data.lastExternalCheck.asString(); // date
			t += ", ";
			t += data.nextExternalCheck.asString(); // date
			t += ", \"";
			t += data.contractDescription;

			// status
			t += "\", ";
			t += data.dateOfSetup.asString(); // date/string?
			t += ", ";
			t += data.dateOfDecommissioning.asString(); // date/string?
			t += ", ";
			t += std::to_string(data.wattage);
			t += ");";

			// execute query
			execQuery(connection, t.c_str());

			// close connection
			sqlite3_close(connection);
			connection = nullptr;
		}
	}
}