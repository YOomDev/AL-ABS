#pragma once

#include <cstdio> // printf_s()
#include <vector> // std::vector<T>
#include <string> // std::string
#include <string.h> // std::to_string()
#include <ctime> // time_t
#include "sqlite/sqlite3.h" // Database library

static const char* createRef = "CREATE TABLE IF NOT EXISTS `reference` (`device_id` INT NOT NULL UNIQUE, `device_name` TEXT NOT NULL, `device_in_use` INT NOT NULL, `device_location` TEXT, `device_costplace` TEXT, `device_next_checkup` BIGINT DEFAULT NULL, PRIMARY KEY (`device_id`));";
static const char* createDev = "CREATE TABLE IF NOT EXISTS `device` (`device_id` INT NOT NULL UNIQUE, `model` TEXT, `serial_number` INT, `supplier` TEXT, `manufacturer` TEXT, `purchase_date` BIGINT, `warranty_date` BIGINT, `department` TEXT, `costplace_name` TEXT, `administrator` TEXT, `replacement` TEXT, `has_log` BIT(1), `has_manual` BIT(1), `fitness_freq` INT, `internal_freq` INT, `last_internal_check` BIGINT, `next_internal_check` BIGINT, `external_company` TEXT, `external_freq` INT, `last_external_check` BIGINT, `next_external_check` BIGINT, `contract_desc` TEXT, `setup_date` BIGINT, `decommission_date` BIGINT, `wattage` FLOAT, PRIMARY KEY (`device_id`));";
static const char* createLog = "CREATE TABLE IF NOT EXISTS `log` (`date` BIGINT NOT NULL UNIQUE, `logger` TEXT NOT NULL, `log` TEXT NOT NULL, PRIMARY KEY (`date`));";
static const std::string unused = "outOfUse";
static const std::string dbCostplace = "db/costplaces/";
static const std::string dbLogs = "db/logs/";
static const std::string dbExtension = ".db";
static const char* dbRef = "db/reference.db";

///////////
// Dates //
///////////

struct Date {
private:
	tm* d = new tm;
	time_t t = time(&t);
public:
	Date() { update(time(0)); };
	Date(time_t e) { update(e); };
	Date(int year, int month, int day) {
		d->tm_year = year - 1900;
		d->tm_mon = month - 1;
		d->tm_mday = day;
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
	const uint64_t asInt64() const { return t; }
	const void fromStr(std::string& str) {
		std::string tmp = str;
		filterDateInput(tmp);
		// check if valid date string
		int lines = 0;
		bool numbersBeforeLine = false;
		for (int i = 0; i < tmp.size(); i++) { if (tmp[i] == '-') { if (!numbersBeforeLine) { return; } lines++; numbersBeforeLine = false; } else { numbersBeforeLine = true; } }
		if (lines != 2 || !numbersBeforeLine) { return; }

		// read date from string
		int next = tmp.find('-');
		int y = std::stoi(tmp.substr(0, next));
		tmp.erase(0, next + 1);
		next = tmp.find('-');
		int m = std::stoi(tmp.substr(0, next));
		tmp.erase(0, next + 1);
		int d = std::stoi(tmp);

		// update string if date is valid
		Date td = Date(y, m, d);
		if (td.t >= 0) { update(td.t); }
		str = asString();
	}

	static const Date getOffset(int y, int m, int d) { return Date(1970 + y, 1 + m, d).t - Date(1970, 1, 1).t; }
	static void filterDateInput(std::string str) {
		std::string tmp;
		int line = 0;
		for (int i = 0; i < str.size(); i++) {
			switch (str[i]) {
				case '-':
					line++;
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
					if (line < 3) {
						tmp += str[i];
					}
					break;
				default: break;
			}
		}
		str = tmp;
	}
};

static const time_t frequencyDateOffset[9]{
	Date::getOffset( 0, 0, 0 + 1).asInt64(),
	Date::getOffset( 0, 0, 1 + 1).asInt64(),
	Date::getOffset( 0, 0, 7 + 1).asInt64(),
	Date::getOffset( 0, 1, 0 + 1).asInt64(),
	Date::getOffset( 0, 3, 0 + 1).asInt64(),
	Date::getOffset( 0, 6, 0 + 1).asInt64(),
	Date::getOffset( 1, 0, 0 + 1).asInt64(),
	Date::getOffset( 5, 0, 0 + 1).asInt64(),
	Date::getOffset(10, 0, 0 + 1).asInt64()
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
		if ((sc = sqlite3_exec(con, t, nullptr, nullptr, nullptr)) != SQLITE_OK) { printf_s("[DB-QUERY-ERROR] %i\n", sc); }
		return sc == SQLITE_OK;
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

	static const void closeConnection(sqlite3*& con) { sqlite3_close(con); con = nullptr; }

	static bool addDevice(const DeviceData& data, bool old) {
		sqlite3* connection = nullptr;

		if (!old) { // dont do this if there is a old entry, DB::moveDevice() updates this already before calling this function
			// Setup data in reference
			if (createConnection(connection, dbRef, createRef)) {
				// Create query
				std::string t = "INSERT INTO `reference` (`device_id`, `device_name`, `device_in_use`, `device_location`, `device_costplace`, `device_next_checkup`) VALUES (";
				t += std::to_string(data.id);
				t += ", \"";
				t += data.name;
				t += "\", ";
				t += data.inUse ? "1" : "0";
				t += ", \"";
				t += data.location;
				t += "\", \"";
				t += data.costplace;
				t += "\", \"";

				time_t tmp = 0;
				if (data.externalFrequency && data.nextExternalCheck.asInt64() > frequencyDateOffset[1]) { tmp = data.nextExternalCheck.asInt64(); }
				if (data.internalFrequency && (tmp == 0 || data.nextInternalCheck.asInt64() < tmp) && data.nextInternalCheck.asInt64() > frequencyDateOffset[1]) { tmp = data.nextInternalCheck.asInt64(); }
				t += std::to_string(tmp);
				t += "\");";

				// Execute query
				if (!execQuery(connection, t.c_str())) { return false; }

				// Close connection
				sqlite3_close(connection);
				connection = nullptr;
			}
		}

		// Setup device info in its used costplace database
		std::string tmp = dbCostplace + (data.inUse ? data.costplace : unused) + dbExtension;
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
			t += std::to_string(data.purchaseDate.asInt64());
			t += ", ";
			t += std::to_string(data.warrantyDate.asInt64());
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
			t += std::to_string(data.lastInternalCheck.asInt64());
			t += ", ";
			t += std::to_string(data.nextInternalCheck.asInt64());

			// external check
			t += ", \"";
			t += data.externalCompany;
			t += "\", ";
			t += std::to_string(data.externalFrequency);
			t += ", ";
			t += std::to_string(data.lastExternalCheck.asInt64());
			t += ", ";
			t += std::to_string(data.nextExternalCheck.asInt64());
			t += ", \"";
			t += data.contractDescription;

			// status
			t += "\", ";
			t += std::to_string(data.dateOfSetup.asInt64());
			t += ", ";
			t += std::to_string(data.dateOfDecommissioning.asInt64());
			t += ", ";
			t += std::to_string(data.wattage);
			t += ");";

			// execute query
			if (!execQuery(connection, t.c_str())) { return false; }

			// close connection
			closeConnection(connection);
		}

		if (!old) {
			// Setup a log database entry file
			tmp = dbLogs + std::to_string(data.id) + dbExtension;
			if (createConnection(connection, tmp.c_str(), createLog)) {
				// create query
				Date today;
				std::string t = "INSERT INTO `log` (`date`, `logger`, `log`) VALUES (" + today.asString() + ", \"ABS\", \"Created device entry\")";

				// execute query
				if (!execQuery(connection, t.c_str())) { return false; }

				// close connection
				sqlite3_close(connection);
				connection = nullptr;
			}
		}
		return true;
	}

	static void addDeviceLog(int id, Date& date, std::string& logger, std::string& log) {
		sqlite3* connection = nullptr;
		std::string tmp = dbLogs + std::to_string(id) + dbExtension;
		if (createConnection(connection, tmp.c_str(), createLog)) {
			// create query
			std::string t = "INSERT INTO `log` (`date`, `logger`, `log`) VALUES (" + date.asString() + ", \"" + logger + "\", \"" + log + "\")";

			// execute query
			execQuery(connection, t.c_str());

			// close connection
			closeConnection(connection);
		}
	}

	static const void updateLog(const int id, const char* t) {
		sqlite3* connection;
		std::string tmp = dbLogs + std::to_string(id) + dbExtension;
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

		if (DB::createConnection(connection, dbRef, createRef)) {
			// Get data from database if there is any
			sqlite3_stmt* stmt;
			sqlite3_prepare_v2(connection, "SELECT device_id, device_name, device_in_use, device_location, device_costplace, device_next_checkup FROM reference ORDER BY device_id ASC", -1, &stmt, NULL);
			while (sqlite3_step(stmt) != SQLITE_DONE) {
				id.push_back(sqlite3_column_int(stmt, 0));
				name.push_back(std::string((const char*)sqlite3_column_text(stmt, 1)));
				inUse.push_back(sqlite3_column_int(stmt, 2) == 1);
				location.push_back(std::string((const char*)sqlite3_column_text(stmt, 3)));
				costplace.push_back(std::string((const char*)sqlite3_column_text(stmt, 4)));
				nextCheckup.push_back(Date(sqlite3_column_int64(stmt, 5)));
			}
			sqlite3_finalize(stmt);

			// Close connection
			DB::closeConnection(connection);
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
	sqlite3* connection = nullptr;
public:

	DeviceMenu() { clearBuffers(); }
	~DeviceMenu() { clearBuffers(); }

	bool isLoaded = false;
	DeviceData data;

	const bool loadDevice(int id, const FilterMenu& filter) {
		// Clear data buffers
		clearBuffers();

		// Get info from filtermenu
		for (int i = 0; i < filter.id.size(); i++) {
			if (filter.id[i] == id) {
				data.id = id;
				data.name = filter.name[i];
				data.inUse = filter.inUse[i];
				data.location = filter.location[i];
				data.costplace = filter.costplace[i];
				break;
			}
		}

		// Read data if it can be found into DeviceData
		std::string tmp = dbCostplace + (data.inUse ? data.costplace : unused) + dbExtension;
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
		tmp = dbLogs + std::to_string(data.id) + dbExtension;
		if (DB::createConnection(connection, tmp.c_str(), createDev)) {
			// Get data from database if there is any
			sqlite3_stmt* stmt;
			tmp = "SELECT date, logger, log FROM `log`";
			sqlite3_prepare_v2(connection, tmp.c_str(), -1, &stmt, NULL);
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
	static void moveDevice(DeviceData& old, DeviceData& edited) {
		sqlite3* connection = nullptr;
		std::string tmp;
		// delete from reference file
		if (createConnection(connection, dbRef, createRef)) {
			time_t t = 0;
			if (edited.externalFrequency && edited.nextExternalCheck.asInt64() > frequencyDateOffset[1]) { t = edited.nextExternalCheck.asInt64(); }
			if (edited.internalFrequency && (t == 0 || edited.nextInternalCheck.asInt64() < t) && edited.nextInternalCheck.asInt64() > frequencyDateOffset[1]) { t = edited.nextInternalCheck.asInt64(); }
			tmp = "UPDATE `reference` SET `device_name`=\"" + edited.name + "\", `device_in_use`=\"" + (edited.inUse ? "1" : "0") + "\", `device_location`=\"" + edited.location + "\", `device_costplace`=\"" + edited.costplace + "\", `device_next_checkup`=\"" + std::to_string(t) + "\" WHERE device_id=" + std::to_string(old.id);
			execQuery(connection, tmp.c_str());
			closeConnection(connection);
		}

		// delete from costplace file
		tmp = dbCostplace + (old.inUse ? old.costplace : unused) + dbExtension;
		if (createConnection(connection, tmp.c_str(), createDev)) {
			tmp = "DELETE FROM `device` WHERE device_id=" + std::to_string(old.id);
			execQuery(connection, tmp.c_str());
			closeConnection(connection);
		}

		addDevice(edited, true);
	}
}