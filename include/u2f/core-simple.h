#pragma once

#include "core.h"

namespace u2f {
	class SimpleCore : public Core {
		virtual void wink();
		virtual bool supportsWink();
		virtual bool isUserPresent();

		virtual bool enroll(const crypto::Hash &applicationHash, Handle &handle, uint8_t &handleSize, crypto::PublicKey &publicKey);
		virtual crypto::Signer* authenticate(const crypto::Hash &applicationHash, const Handle &handle, uint8_t handleSize, bool checkUserPresence, bool &userPresent, uint32_t &authCounter);
		virtual crypto::Signer* getAttestationSigner();
	};
}
