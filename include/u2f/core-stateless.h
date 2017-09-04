#pragma once

#include <u2f/core-simple.h>
#include <sqlite3.h>

namespace u2f {

	/**
	 * Much like UnsafeCore, this U2F core stores applicationHash abd private keys into the key handle.
	 *
	 * Unlike it, though,
	 * in the handle, but properly uses encryption.
	 */
	class StatelessCore : public SimpleCore {
		uint32_t aesKey[60];

	public:
		StatelessCore(const char* password);

		virtual bool createHandle(const crypto::Hash &applicationHash, const crypto::PrivateKey &privateKey, Handle &handle, uint8_t &handleSize);
		virtual bool fetchHandle(const crypto::Hash &applicationHash, const Handle &handle, uint8_t handleSize, crypto::PrivateKey &privateKey, uint32_t &authCounter);
	};
}
