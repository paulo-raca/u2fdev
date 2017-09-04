#pragma once

#include <u2f/crypto.h>

namespace u2f {
	typedef uint8_t Handle[255];

	class Core {
	private:
		uint16_t processRequest(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, const uint8_t *request, uint32_t requestSize, uint8_t *response, uint32_t &responseSize);
		uint16_t processRegisterRequest(const uint8_t *request, uint32_t requestSize, uint8_t *response, uint32_t &responseSize);
		uint16_t processAuthenticationRequest(uint8_t control, const uint8_t *request, uint32_t requestSize, uint8_t *response, uint32_t &responseSize);
	public:
		virtual bool processRawAdpu(const uint8_t* rawRequest, uint32_t rawRequestSize, const uint8_t *&rawResponse, uint32_t& rawResponseSize);

		virtual bool supportsWink();
		virtual void wink();

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
		virtual bool enroll(const crypto::Hash &applicationHash, Handle &handle, uint8_t &handleSize, crypto::PublicKey &publicKey) = 0;

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
		virtual crypto::Signer* authenticate(const crypto::Hash &applicationHash, const Handle &handle, uint8_t handleSize, bool checkUserPresence, bool &userPresent, uint32_t &authCounter) = 0;

		/**
		 * Returns the signer used for attestation of the registration.
		 *
		 * @return The signer used to attestate the registration. It must be deleted by the caller.
		 */
		virtual crypto::Signer* getAttestationSigner() = 0;
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
