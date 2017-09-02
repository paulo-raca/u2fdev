#pragma once

#include <inttypes.h>

namespace u2f {
	typedef uint8_t Hash[32];
	typedef uint8_t PrivateKey[32];
	typedef uint8_t PublicKey[65];
	typedef uint8_t Signature[73];
	typedef uint8_t Handle[255];

	class Crypto {
	public:
		static void sha256(u2f::Hash &hash, ...);
		static bool makeKeyPair(PublicKey &publicKey, PrivateKey &privateKey);
		static bool sign(const PrivateKey &privateKey, const Hash &messageHash, u2f::Signature &signature);
		static uint8_t signatureSize(const Signature &signature);
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

		//For a given applicationHash, register a privateKey and returns a handle
		virtual void createHandle(const uint8_t *applicationHash, const uint8_t *privateKey, uint8_t *handle, uint8_t &handleSize);
		//For a given applicationHash and handle, fetch the privateKey. Returns false if the handle is invalid
		virtual bool fetchHandle(const uint8_t *applicationHash, const uint8_t *handle, uint8_t handleSize, uint8_t *privateKey);

		/**
		 * Returns the attestation certificate as a PEM buffer
		 *
		 * @param[out] certificate Will point to a buffer with the PEM certificate
		 * @param[out] certificateSize Will be set with the size of #certificate
		 */
		virtual void getAttestationCertificate(const uint8_t *&certificate, uint16_t &certificateSize);

		/**
		 * Signs the messageHash with the attestation private key
		 *
		 * @param[in] messageHash SHA256 of the message being signed
		 * @param[out] signature Buffer where the signature will be stored
		 */
		virtual bool attestationSign(const Hash &messageHash, Signature &signature);

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
