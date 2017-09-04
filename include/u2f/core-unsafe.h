#pragma once

#include <u2f/core-simple.h>

namespace u2f {

	/**
	 * This is the simplest possible core:
	 * - User Handles are [applicationHash, privateKey]
	 * - authCounter is the timestamp
	 *
	 * As you may guess, it's unsafe and serves only as a demo.
	 */
	class UnsafeCore : public SimpleCore {
	public:
		virtual bool createHandle(const crypto::Hash &applicationHash, const crypto::PrivateKey &privateKey, Handle &handle, uint8_t &handleSize);
		virtual bool fetchHandle(const crypto::Hash &applicationHash, const Handle &handle, uint8_t handleSize, crypto::PrivateKey &privateKey, uint32_t &authCounter);
	};
}
