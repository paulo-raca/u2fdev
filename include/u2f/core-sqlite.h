#pragma once

#include <u2f/core-simple.h>
#include <sqlite3.h>

namespace u2f {

	/**
	 * This U2F core uses an SQLite database to store handles and private keys.
	 *
	 * It has some advantages:
	 * - Handles contain no private key data (Encrypted or not), and are completely safe.
	 * - The authentication counter is applied per-key.
	 *
	 * On the other handm, it requires a reasonable amount of storage and is therefore not suitable for tiny embedded systems.
	 */
	class SQLiteCore : public SimpleCore {
		sqlite3 *db;
	public:
		SQLiteCore(const char* filename);
		~SQLiteCore();

		virtual bool createHandle(const crypto::Hash &applicationHash, const crypto::PrivateKey &privateKey, Handle &handle, uint8_t &handleSize);
		virtual bool fetchHandle(const crypto::Hash &applicationHash, const Handle &handle, uint8_t handleSize, crypto::PrivateKey &privateKey, uint32_t &authCounter);
	};
}
