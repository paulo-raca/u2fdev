#pragma once

#include "core.h"

namespace u2f {
	/**
	 * The simplest possible working implementation of a U2F Core.
	 *
	 * This is means as an example of how to implement a Core, but has a number of issues:
	 * - It assumes the user is always present (Unless you override isUserPresent)
	 * - It is very unsecure: The handle is composed of [applicationHash, privateKey], unencrypted.
	 * - The authenticationCounter is stored in memory and zeros whenever the program restarts.
	 */
	class SimpleCore : public Core {
		virtual bool isUserPresent();

		virtual bool enroll(const crypto::Hash &applicationHash, Handle &handle, uint8_t &handleSize, crypto::PublicKey &publicKey);
		virtual crypto::Signer* authenticate(const crypto::Hash &applicationHash, const Handle &handle, uint8_t handleSize, bool checkUserPresence, bool &userPresent, uint32_t &authCounter);
		virtual crypto::Signer* getAttestationSigner();
	};
}
