#include <u2f/core-sqlite.h>
#include <sqlite3.h>
#include <string.h>
#include <stdio.h>

#define LOG(fmt, ...) fprintf(stderr, "u2f-core-sqlite: " fmt "\n", ##__VA_ARGS__)

u2f::SQLiteCore::SQLiteCore(const char* filename) {
	// Open the DB
	int ret = sqlite3_open(filename, &db);
	if (ret != SQLITE_OK) {
		LOG("Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		db = nullptr;
	}

	// Setup the table
	ret = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS Handle ("
			"	applicationHash BLOB,"
			"	handle BLOB,"
			"	privateKey BLOB,"
			"	authCounter INTEGER DEFAULT 0,"
			"	PRIMARY KEY (applicationHash, handle)"
			");",
			nullptr, nullptr, nullptr);

	if (ret != SQLITE_OK) {
		LOG("Can'create table Handle: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		db = nullptr;
	}
}

u2f::SQLiteCore::~SQLiteCore() {
	if (db) {
		sqlite3_close(db);
	}
}

bool u2f::SQLiteCore::createHandle(const crypto::Hash &applicationHash, const crypto::PrivateKey &privateKey, Handle &handle, uint8_t &handleSize) {
	if (!db)
		return false; // Database is closed

	//Create a new random handle
	handleSize = 64;
	sqlite3_randomness(handleSize, handle);


	sqlite3_stmt *stmt = nullptr;
	int ret = sqlite3_prepare_v2(db, "INSERT INTO Handle (applicationHash, handle, privateKey) VALUES (?1, ?2, ?3);", -1, &stmt, nullptr);
	if (ret != SQLITE_OK) {
		LOG("Failed prepare Insert statement: %s\n", sqlite3_errmsg(db));
		return false;
	}
	sqlite3_bind_blob(stmt, 1, applicationHash, sizeof(crypto::Hash), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, handle, handleSize, SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 3, privateKey, sizeof(crypto::PrivateKey), SQLITE_STATIC);

	ret = sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	if (ret != SQLITE_DONE) {
		LOG("Failed to insert handle: %s\n", sqlite3_errmsg(db));
		return false;
	}
	return true;
}

bool u2f::SQLiteCore::fetchHandle(const u2f::crypto::Hash &applicationHash, const u2f::Handle &handle, uint8_t handleSize, u2f::crypto::PrivateKey &privateKey, uint32_t &authCounter) {
	if (!db)
		return false; // Database is closed

	sqlite3_stmt *stmt = nullptr;
	int ret = sqlite3_prepare_v2(db, "Select privateKey, authCounter FROM Handle WHERE applicationHash = ?1 AND handle = ?2;", -1, &stmt, nullptr);
	if (ret != SQLITE_OK) {
		LOG("Failed prepare 'select handle' statement: %s\n", sqlite3_errmsg(db));
		return false;
	}
	sqlite3_bind_blob(stmt, 1, applicationHash, sizeof(crypto::Hash), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, handle, handleSize, SQLITE_STATIC);

	ret = sqlite3_step(stmt);
	if (ret == SQLITE_DONE) {
		// Handle not found ¯\_(ツ)_/¯
		sqlite3_finalize(stmt);
		return false;
	} else if (ret != SQLITE_ROW) {
		//Some error?!
		LOG("Failed select handle: %s\n", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return false;
	}

	// Output privateKey and authCounter
	memcpy(privateKey, sqlite3_column_blob(stmt, 0), sizeof(crypto::PrivateKey));
	authCounter = sqlite3_column_int(stmt, 1);
	sqlite3_finalize(stmt);

	// Increment authCounter
	ret = sqlite3_prepare_v2(db, "UPDATE Handle SET authCounter = authCounter + 1 WHERE applicationHash = ?1 AND handle = ?2;", -1, &stmt, nullptr);
	if (ret != SQLITE_OK) {
		LOG("Failed prepare 'update authCounter' statement: %s\n", sqlite3_errmsg(db));
		return false;
	}
	sqlite3_bind_blob(stmt, 1, applicationHash, sizeof(crypto::Hash), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, handle, handleSize, SQLITE_STATIC);
	ret = sqlite3_step(stmt);
	if (ret != SQLITE_DONE) {
		LOG("Failed to update authCounter: %s\n", sqlite3_errmsg(db));
		return false;
	}

	LOG("counter = %d", authCounter);
	return true;
}
