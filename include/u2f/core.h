#pragma once

#include <inttypes.h>

namespace u2f {
	typedef uint8_t Hash[32];
	typedef uint8_t PrivateKey[32];
	typedef uint8_t PublicKey[65];
	typedef uint8_t Signature[73];
	typedef uint8_t Handle[255];

	enum class SignCondition {
		Never = 0x07,
		Always = 0x08,
		RequiresUserPresence = 0x03
	};

	namespace Crypto {
		void sha256(u2f::Hash &hash, ...);
		bool makeKeyPair(PublicKey &publicKey, PrivateKey &privateKey);
		bool sign(const PrivateKey &privateKey, const Hash &messageHash, u2f::Signature &signature);
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
			virtual bool sign(const Hash &messageHash, u2f::Signature &signature) = 0;

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
			SimpleSigner(const PrivateKey &privateKey)
			: privateKey(privateKey), certificate(nullptr), certificateSize(0)
			{}

			SimpleSigner(const PrivateKey &privateKey, const uint8_t *certificate, uint16_t certificateSize)
			: privateKey(privateKey), certificate(certificate), certificateSize(certificateSize)
			{}

			virtual bool sign(const Hash &messageHash, u2f::Signature &signature) {
				return Crypto::sign(privateKey, messageHash, signature);
			}
			virtual bool getCertificate(const uint8_t *&certificate, uint16_t &certificateSize) {
				certificate = this->certificate;
				certificateSize = this->certificateSize;
				return certificate != nullptr;
			}
		};
	};


	class Core {
	public:
		virtual bool processRawAdpu(const uint8_t* rawRequest, uint32_t rawRequestSize, const uint8_t *&rawResponse, uint32_t& rawResponseSize);
		uint16_t processRequest(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, const uint8_t *request, uint32_t requestSize, uint8_t *response, uint32_t &responseSize);
		uint16_t processRegisterRequest(const uint8_t *request, uint32_t requestSize, uint8_t *response, uint32_t &responseSize);
		uint16_t processAuthenticationRequest(uint8_t control, const uint8_t *request, uint32_t requestSize, uint8_t *response, uint32_t &responseSize);
		virtual void wink();
		virtual bool supportsWink();
		virtual bool isUserPresent();

		/**
		 * Creates a new handle.
		 *
		 * @param[in]  applicationHash Identifies the application bound to this handle.
		 * @param[out] handle This buffer will be filled with the new handle.
		 * @param[out] handleSize Size of #handle.
		 * @param[out] publicKey The public key associated with the new #handle.
		 *
		 * @return true if a new handle was enrolled, false if failed because the user isn't present.
		 */
		virtual bool enroll(
				const Hash &applicationHash,
				Handle &handle,
				uint8_t &handleSize,
				PublicKey &publicKey);

		/**
		 * Authenticates a handle.
		 *
		 * @param[in]  applicationHash Identifies the application.
		 * @param[in]  handle Identifies the handle.
		 * @param[in]  handleSize Size of #handle.
		 * @param[in]  checkUserPresence If #userPresent should be populated.
		 * @param[out] userPresent Indicates if the the user is present. (Only set if the handle is valid and #checkUserPresence is set)
		 * @param[out] authCounter Monotonic counter of the number of times this handler (or this device) has been used. (Only set if the handle is valid)
		 *
		 * @return nullptr if the handle is invalid, or the the signer used to sign the authentication requests. It must be deleted by the caller.
		 */
		virtual Crypto::Signer* authenticate(const Hash &applicationHash, const Handle &handle, uint8_t handleSize, bool checkUserPresence, bool &userPresent, uint32_t &authCounter);

		/**
		 * Returns the signer used for attestation of the registration.
		 *
		 * @return The signer used to attestate the registration. It must be deleted by the caller.
		 */
		virtual Crypto::Signer* getAttestationSigner();
	};

	class Protocol {
	protected:
		Core& core;

	public:
		inline Protocol(Core& core)
			: core(core)
		{ }
	};

}
