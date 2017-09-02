#include <u2f/core.h>
#include <stdio.h>
#include <string.h>

#define LOG(fmt, ...) fprintf(stderr, "u2f-core: " fmt "\n", ##__VA_ARGS__)

#define SW_NO_ERROR                 0x9000 // The command completed successfully without error.
#define SW_CONDITIONS_NOT_SATISFIED 0x6985 // The request was rejected due to test-of-user-presence being required.
#define SW_WRONG_DATA               0x6A80 // The request was rejected due to an invalid key handle.
#define SW_WRONG_LENGTH             0x6700 // The length of the request was invalid.
#define SW_CLA_NOT_SUPPORTED        0x6E00 // The Class byte of the request is not supported.
#define SW_INS_NOT_SUPPORTED        0x6D00 //The Instruction of the request is not supported.

#define INS_REGISTER                  0x01
#define INS_AUTHENTICATE              0x02
#define INS_VERSION                   0x03

#define AUTH_CHECK_ONLY               0x07
#define AUTH_ENFORCE_USER_SIGN        0x03
#define AUTH_DONT_ENFORCE_USER_SIGN   0x08











bool u2f::Core::processRawAdpu(const uint8_t* rawRequest, uint32_t rawRequestSize, const uint8_t *&rawResponse, uint32_t& rawResponseSize) {
	if (rawRequestSize < 4)
		return false;

	uint8_t cla = rawRequest[0];
	uint8_t ins = rawRequest[1];
	uint8_t p1 = rawRequest[2];
	uint8_t p2 = rawRequest[3];

	const uint8_t* request = nullptr;
	uint32_t requestSize = 0;

	uint8_t* response = nullptr;
	uint32_t responseSize = 0;

	if (rawRequestSize == 4) {
		// Valid request, with empty request and response
	} else if (rawRequestSize < 7) {
		// Invalid size
		return false;
	} else if (rawRequestSize == 7) {
		//Only response size
		if (rawRequest[4] != 0)
			return false;

		responseSize = (rawRequest[5] << 8) | (rawRequest[6] << 0);
		if (responseSize == 0)
			responseSize = 0x10000;
	} else {
		//Non-empty request, maybe a response size
		if (rawRequest[4] != 0)
			return false;

		requestSize = (rawRequest[5] << 8) | (rawRequest[6] << 0);
		request = rawRequest + 7;

		if (requestSize == rawRequestSize - 7) {
			//Valid, empty response
		} else if (requestSize == rawRequestSize - 9) {
			//Payload followed by response size
			responseSize = (rawRequest[7+requestSize] << 8) | (rawRequest[8+requestSize] << 0);
			if (responseSize == 0)
				responseSize = 0x10000;
		} else {
			//Invalid
			return false;
		}
	}

	response = new uint8_t[responseSize+2];
	uint16_t sw = processRequest(cla, ins, p1, p2, request, requestSize, response, responseSize);
	response[responseSize  ] = (sw >> 8);
	response[responseSize+1] = (sw >> 0);

	rawResponse = response;
	rawResponseSize = responseSize + 2;

	return true;
}

uint16_t u2f::Core::processRequest(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, const uint8_t *request, uint32_t requestSize, uint8_t *response, uint32_t &responseSize) {
	if (cla != 0) {
		LOG("Unknown CLA: %d", cla);
		responseSize = 0;
		return SW_CLA_NOT_SUPPORTED;
	}

	switch (ins) {
		case INS_REGISTER: {
			LOG("Register");
			return processRegisterRequest(request, requestSize, response, responseSize);
		}

		case INS_AUTHENTICATE: {
			LOG("Authenticate - %d", p1);
			return processAuthenticationRequest(p1, request, requestSize, response, responseSize);
		}

		case INS_VERSION: {
			LOG("Version");
			if (responseSize < 6) {
				responseSize = 0;
				return SW_WRONG_LENGTH;
			}
			memcpy(response, "U2F_V2", 6);
			responseSize = 6;
			return SW_NO_ERROR;
		}

		default: {
			LOG("Unknown INS:%d", ins);
			responseSize = 0;
			return SW_INS_NOT_SUPPORTED;
		}
	}
}

uint16_t u2f::Core::processRegisterRequest(const uint8_t *request, uint32_t requestSize, uint8_t *response, uint32_t &responseSize) {
	if (requestSize != 64) {
		LOG("Register request with wrong length: %d", requestSize);
		responseSize = 0;
		return SW_WRONG_LENGTH;
	}

	Hash &challengeHash = *(Hash*)&request[0];
	Hash &applicationHash = *(Hash*)&request[32];

	responseSize = 0;

	//Reserved byte
	uint8_t &responseReserved = response[responseSize++];
	responseReserved = 0x05;

	// Public key
	PublicKey &responsePublicKey = *(PublicKey*)&response[responseSize];
	responseSize += sizeof(PublicKey);

	// Handle
	uint8_t &responseKeyHandleSize = response[responseSize++];
	Handle &responseKeyHandle = *(Handle*)&response[responseSize];

	if (!enroll(applicationHash, responseKeyHandle, responseKeyHandleSize, responsePublicKey)) {
		//Failed to create a key, probably because the user isn't present
		responseSize = 0;
		return SW_CONDITIONS_NOT_SATISFIED;
	}

	responseSize += responseKeyHandleSize;

	// Attestation
	Crypto::Signer *attestationSigner = getAttestationSigner();

	// Copy the attestation Certificate to the response
	const uint8_t *attestationCertificate = nullptr;
	uint16_t attestationCertificateSize = 0;
	attestationSigner->getCertificate(attestationCertificate, attestationCertificateSize);

	uint8_t *responseAttestationCertificate = &response[responseSize];
	memcpy(responseAttestationCertificate, attestationCertificate, attestationCertificateSize);
	responseSize += attestationCertificateSize;

	// Calculates the challenge hash
	Hash hash;
	uint8_t hash_reserved = 0;
	Crypto::sha256(
		hash,
		&hash_reserved, 1,
		applicationHash, sizeof(Hash),
		challengeHash, sizeof(Hash),
		responseKeyHandle, responseKeyHandleSize,
		&responsePublicKey, sizeof(PublicKey),
		nullptr);

	//Sign the challenge
	Signature &responseSignature = *(Signature*)&response[responseSize];
	attestationSigner->sign(hash, responseSignature);
	responseSize += Crypto::signatureSize(responseSignature);

	delete attestationSigner;
	return SW_NO_ERROR;
}

uint16_t u2f::Core::processAuthenticationRequest(uint8_t control, const uint8_t *request, uint32_t requestSize, uint8_t *response, uint32_t &responseSize) {
	SignCondition signCondition = (SignCondition)control;
	if (signCondition != SignCondition::Always && signCondition != SignCondition::Never && signCondition != SignCondition::RequiresUserPresence) {
		LOG("Authenticate - Invalid sign condition %d", (uint8_t)signCondition);
		responseSize = 0;
		return SW_WRONG_DATA;
	}

	if (requestSize < 65) {
		LOG("Authenticate - Request size is too small: %d", requestSize);
		responseSize = 0;
		return SW_WRONG_LENGTH;
	}

	Hash &challengeHash = *(Hash*)&request[0];
	Hash &applicationHash = *(Hash*)&request[32];
	uint8_t handleSize = request[64];
	Handle &handle = *(Handle*)&request[65];
	bool userPresent = false;
	uint32_t authCounter = 0;

	if (requestSize != handleSize + 65) {
		LOG("Authenticate - Request size should be %d, was %d", handleSize + 65, requestSize);
		responseSize = 0;
		return SW_WRONG_LENGTH;
	}

	Crypto::Signer *signer = authenticate(applicationHash, handle, handleSize, signCondition != SignCondition::Never, userPresent, authCounter);

	// Check for invalid Handle
	if (signer == nullptr) {
		LOG("Authenticate - Invalid handle");
		responseSize = 0;
		return SW_WRONG_DATA;
	}

	// Check for user not present
	if ((signCondition == SignCondition::Never) || ((signCondition == SignCondition::RequiresUserPresence) && !userPresent)) {
		LOG("Authenticate - User not present");
		responseSize = 0;
		delete signer;
		return SW_CONDITIONS_NOT_SATISFIED;
	}

	responseSize = 0;
	response[responseSize++] = userPresent ? 1 : 0;
	response[responseSize++] = authCounter >> 24;
	response[responseSize++] = authCounter >> 16;
	response[responseSize++] = authCounter >>  8;
	response[responseSize++] = authCounter >>  0;

	// Perform signature
	Hash hash;
	Crypto::sha256(
		hash,
		applicationHash, sizeof(Hash),
		response, responseSize,
		challengeHash, sizeof(Hash),
		nullptr);

	Signature &signature = *(Signature*)&response[responseSize];
	signer->sign(hash, signature);
	delete signer;
	responseSize += Crypto::signatureSize(signature);

	LOG("Authenticate - Success");
	return SW_NO_ERROR;
}

void u2f::Core::wink() {
}

bool u2f::Core::supportsWink() {
	return false;
}
