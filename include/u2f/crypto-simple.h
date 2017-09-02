#pragma once

#include "core.h"

namespace u2f {
	namespace crypto {
		/**
		 * Simplest implementation of a Signer, just wraps static buffers with the private key and certificate.
		 *
		 * It doesn't do any memory management -- The buffers aren't copied or freed anywhere.
		 */
		class SimpleSigner : public Signer {
			const PrivateKey &privateKey;
			const uint8_t *certificate;
			const uint16_t certificateSize;
		public:
			SimpleSigner(const PrivateKey &privateKey);

			SimpleSigner(const PrivateKey &privateKey, const uint8_t *certificate, uint16_t certificateSize);

			virtual bool sign(const Hash &messageHash, Signature &signature);

			virtual bool getCertificate(const uint8_t *&certificate, uint16_t &certificateSize);
		};
	};
}
