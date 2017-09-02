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

bool u2f::Core::enroll(const Hash &applicationHash, Handle &handle, uint8_t &handleSize, PublicKey &publicKey) {
	// A user is required
	if (!isUserPresent()) {
		return false;
	}

	// Create the handle
	handleSize = sizeof(Hash) + sizeof(PrivateKey);
	Hash &handleApplicationHash = *(Hash*)&handle[0];
	PrivateKey& handlePrivateKey = *(PrivateKey*)&handle[sizeof(Hash)];

	//1st part of the handle is applicationHash
	memcpy(handleApplicationHash, applicationHash, sizeof(Hash));

	//Create the keypair
	if (!Crypto::makeKeyPair(publicKey, handlePrivateKey)) {
		//Failed to create a key.
		//I guess this shoudn't happen?
		return false;
	}

	return true; // Enrolled!
}

u2f::Crypto::Signer* u2f::Core::authenticate(const Hash &applicationHash, const Handle &handle, uint8_t handleSize, bool checkUserPresence, bool &userPresent, uint32_t &authCounter) {
	static uint32_t _authcounter = 0; // This sucks, the counter should not be reset at every launch

	// Check for a valid handle
	if (handleSize != sizeof(Hash) + sizeof(PrivateKey))
		return nullptr;
	const Hash& handleApplicationHash = *(Hash*)&handle[0];
	const PrivateKey& handlePrivateKey = *(PrivateKey*)&handle[32];

	// Ensure the applicationHash matches
	if (memcmp(applicationHash, handleApplicationHash, sizeof(Hash)))
		return nullptr;

	// Check for user presence
	if (checkUserPresence) {
		userPresent = isUserPresent();
	}

	// Get the authentication counter
	authCounter = _authcounter++;

	// Return the Signer
	return new Crypto::SimpleSigner(handlePrivateKey);
}

u2f::Crypto::Signer* u2f::Core::getAttestationSigner() {
	static const PrivateKey privateKey = {
		0xf3, 0xfc, 0xcc, 0x0d, 0x00, 0xd8, 0x03, 0x19, 0x54, 0xf9, 0x08, 0x64, 0xd4, 0x3c, 0x24, 0x7f,
		0x4b, 0xf5, 0xf0, 0x66, 0x5c, 0x6b, 0x50, 0xcc, 0x17, 0x74, 0x9a, 0x27, 0xd1, 0xcf, 0x76, 0x64
	};
	static const uint8_t certificate[] = {
		0x30, 0x82, 0x01, 0x3c, 0x30, 0x81, 0xe4, 0xa0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x0a, 0x47, 0x90,
		0x12, 0x80, 0x00, 0x11, 0x55, 0x95, 0x73, 0x52, 0x30, 0x0a, 0x06, 0x08, 0x2a, 0x86, 0x48, 0xce,
		0x3d, 0x04, 0x03, 0x02, 0x30, 0x17, 0x31, 0x15, 0x30, 0x13, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13,
		0x0c, 0x47, 0x6e, 0x75, 0x62, 0x62, 0x79, 0x20, 0x50, 0x69, 0x6c, 0x6f, 0x74, 0x30, 0x1e, 0x17,
		0x0d, 0x31, 0x32, 0x30, 0x38, 0x31, 0x34, 0x31, 0x38, 0x32, 0x39, 0x33, 0x32, 0x5a, 0x17, 0x0d,
		0x31, 0x33, 0x30, 0x38, 0x31, 0x34, 0x31, 0x38, 0x32, 0x39, 0x33, 0x32, 0x5a, 0x30, 0x31, 0x31,
		0x2f, 0x30, 0x2d, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x26, 0x50, 0x69, 0x6c, 0x6f, 0x74, 0x47,
		0x6e, 0x75, 0x62, 0x62, 0x79, 0x2d, 0x30, 0x2e, 0x34, 0x2e, 0x31, 0x2d, 0x34, 0x37, 0x39, 0x30,
		0x31, 0x32, 0x38, 0x30, 0x30, 0x30, 0x31, 0x31, 0x35, 0x35, 0x39, 0x35, 0x37, 0x33, 0x35, 0x32,
		0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a,
		0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0x8d, 0x61, 0x7e, 0x65, 0xc9,
		0x50, 0x8e, 0x64, 0xbc, 0xc5, 0x67, 0x3a, 0xc8, 0x2a, 0x67, 0x99, 0xda, 0x3c, 0x14, 0x46, 0x68,
		0x2c, 0x25, 0x8c, 0x46, 0x3f, 0xff, 0xdf, 0x58, 0xdf, 0xd2, 0xfa, 0x3e, 0x6c, 0x37, 0x8b, 0x53,
		0xd7, 0x95, 0xc4, 0xa4, 0xdf, 0xfb, 0x41, 0x99, 0xed, 0xd7, 0x86, 0x2f, 0x23, 0xab, 0xaf, 0x02,
		0x03, 0xb4, 0xb8, 0x91, 0x1b, 0xa0, 0x56, 0x99, 0x94, 0xe1, 0x01, 0x30, 0x0a, 0x06, 0x08, 0x2a,
		0x86, 0x48, 0xce, 0x3d, 0x04, 0x03, 0x02, 0x03, 0x47, 0x00, 0x30, 0x44, 0x02, 0x20, 0x60, 0xcd,
		0xb6, 0x06, 0x1e, 0x9c, 0x22, 0x26, 0x2d, 0x1a, 0xac, 0x1d, 0x96, 0xd8, 0xc7, 0x08, 0x29, 0xb2,
		0x36, 0x65, 0x31, 0xdd, 0xa2, 0x68, 0x83, 0x2c, 0xb8, 0x36, 0xbc, 0xd3, 0x0d, 0xfa, 0x02, 0x20,
		0x63, 0x1b, 0x14, 0x59, 0xf0, 0x9e, 0x63, 0x30, 0x05, 0x57, 0x22, 0xc8, 0xd8, 0x9b, 0x7f, 0x48,
		0x88, 0x3b, 0x90, 0x89, 0xb8, 0x8d, 0x60, 0xd1, 0xd9, 0x79, 0x59, 0x02, 0xb3, 0x04, 0x10, 0xdf
	};
	return new Crypto::SimpleSigner(privateKey, certificate, sizeof(certificate));
}

bool u2f::Core::isUserPresent() {
	return true;
}

void u2f::Core::wink() {
	printf(";)\n");
}
bool u2f::Core::supportsWink() {
	return true;
}
