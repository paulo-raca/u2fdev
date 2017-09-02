#pragma once

#include <inttypes.h>

namespace u2f {
	namespace crypto {
		typedef uint8_t Hash[32];
		typedef uint8_t PrivateKey[32];
		typedef uint8_t PublicKey[65];
		typedef uint8_t Signature[73];

		void sha256(Hash &hash, ...);
		bool makeKeyPair(PublicKey &publicKey, PrivateKey &privateKey);
		bool sign(const PrivateKey &privateKey, const Hash &messageHash, Signature &signature);
		uint8_t signatureSize(const Signature &signature);

		/**
		 * Performs signatures of pre-hashed buffers.
		 *
		 * This extra indirection layer allows you to be extra-paranoid with your private keys -- Use a crypto-chip and whatnot.
		 *
		 * Optionally, you can also retrieve a PEM certificate with the public key.
		 */
		class Signer {
		public:
			/**
			* Signs the messageHash with the private key.
			*
			* @param[in] messageHash SHA256 of the message being signed
			* @param[out] signature Buffer where the signature will be stored
			*
			* @return true if the signature was successfully created.
			*/
			virtual bool sign(const Hash &messageHash, Signature &signature) = 0;

			/**
			* Returns the attestation certificate as a PEM buffer.
			*
			* The buffer is valid until the Signer object is destructed and should not be freed by the caller.
			*
			* @param[out] certificate Will point to a buffer with the PEM certificate
			* @param[out] certificateSize Will be set with the size of #certificate
			*/
			virtual bool getCertificate(const uint8_t *&certificate, uint16_t &certificateSize) = 0;
		};
	};
}
