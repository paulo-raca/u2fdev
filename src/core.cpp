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
	if (!isUserPresent()) {
		responseSize = 0;
		return SW_CONDITIONS_NOT_SATISFIED;
	}

	const uint8_t *challengeHash = request;
	const uint8_t *applicationHash = request+32;

	responseSize = 0;

	//Reserved byte
	uint8_t &responseReserved = response[responseSize];
	responseReserved = 0x05;
	responseSize++;

	//Public key
	PublicKey &responsePublicKey = *(PublicKey*)&response[responseSize];
	responseSize += sizeof(PublicKey);

	PrivateKey privateKey;

	if (!Crypto::makeKeyPair(responsePublicKey, privateKey)) {
		//Failed to create a key.
		//I guess this shoudn't happen?
		responseSize = 0;
		return SW_CONDITIONS_NOT_SATISFIED;
	}

	//Key Handle
	uint8_t &responseKeyHandleSize = response[responseSize];
	responseSize++;

	uint8_t *responseKeyHandle = &response[responseSize];
	createHandle(applicationHash, privateKey, responseKeyHandle, responseKeyHandleSize);
	responseSize += responseKeyHandleSize;

	//Certificate
	uint8_t *responseAttestationCertificate = &response[responseSize];

	const uint8_t *attestationPrivateKey = nullptr;
	const uint8_t *attestationCertificateDer = nullptr;
	uint16_t attestationCertificateDerSize = 0;
	getAttestationCertificate(attestationCertificateDer, attestationCertificateDerSize);

	memcpy(responseAttestationCertificate, attestationCertificateDer, attestationCertificateDerSize);
	responseSize += attestationCertificateDerSize;

	//Signature
	Signature &responseSignature = *(Signature*)&response[responseSize];
	{
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

		attestationSign(hash, responseSignature);
		responseSize += Crypto::signatureSize(responseSignature);;
	}
	return SW_NO_ERROR;
}

uint16_t u2f::Core::processAuthenticationRequest(uint8_t control, const uint8_t *request, uint32_t requestSize, uint8_t *response, uint32_t &responseSize) {
	if (control != AUTH_CHECK_ONLY && control != AUTH_ENFORCE_USER_SIGN && control != AUTH_DONT_ENFORCE_USER_SIGN) {
		LOG("Authenticate - Invalid control byte %d", control);
		responseSize = 0;
		return SW_WRONG_DATA;
	}

	if (requestSize < 65) {
		LOG("Authenticate - RequestSize is too small: %d", requestSize);
		responseSize = 0;
		return SW_WRONG_LENGTH;
	}

	const uint8_t *challengeHash = request;
	const uint8_t *applicationHash = request+32;
	uint8_t handleSize = request[64];
	const uint8_t *handle = request+65;
	bool userPresent = isUserPresent();

	if (requestSize != handleSize + 65) {
		LOG("Authenticate - RequestSize should be %d, was %d", handleSize + 65, requestSize);
		responseSize = 0;
		return SW_WRONG_LENGTH;
	}

	uint8_t privateKey[32];
	if (!fetchHandle(applicationHash, handle, handleSize, privateKey)) {
		LOG("Authenticate - Invalid handle");
		responseSize = 0;
		return SW_WRONG_DATA;
	}

	//
	if ((control == AUTH_CHECK_ONLY) || (control == AUTH_ENFORCE_USER_SIGN && !userPresent)) {
		LOG("Authenticate - User not present");
		responseSize = 0;
		return SW_CONDITIONS_NOT_SATISFIED;
	}


	responseSize = 0;
	response[responseSize++] = 56;
	static uint32_t counter = 0; //FIXME
	counter++;
	response[responseSize++] = counter >> 24;
	response[responseSize++] = counter >> 16;
	response[responseSize++] = counter >>  8;
	response[responseSize++] = counter >>  0;

	Signature &signature = *(Signature*)&response[responseSize];

	Hash hash;
	Crypto::sha256(
		hash,
		applicationHash, 32,
		response, 5,
		challengeHash, 32,
		nullptr);

	Crypto::sign(privateKey, hash, signature);
	responseSize += Crypto::signatureSize(signature);

	LOG("Authenticate - Success");
	return SW_NO_ERROR;
}



// Not exactly secure, but it is only a test implementation.
void u2f::Core::createHandle(const uint8_t *applicationHash, const uint8_t *privateKey, uint8_t *handle, uint8_t &handleSize) {
	memcpy(handle   , applicationHash, 32);
	memcpy(handle+32, privateKey, 32);
	handleSize = 64;
}

bool u2f::Core::fetchHandle(const uint8_t *applicationHash, const uint8_t *handle, uint8_t handleSize, uint8_t *privateKey) {
	if (handleSize != 64)
		return false; // Wrong size
	if (memcmp(applicationHash, handle, 32))
		return false; // Wrong applications

	memcpy(privateKey, handle+32, 32);
	return true;
}

void u2f::Core::getAttestationCertificate(const uint8_t *&certificate, uint16_t &certificateSize) {
	static const uint8_t savedCertificate[] = {
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

	certificate = savedCertificate;
	certificateSize = sizeof(savedCertificate);
}

bool u2f::Core::attestationSign(const Hash &messageHash, Signature &signature) {
	PrivateKey privateKey = {
		0xf3, 0xfc, 0xcc, 0x0d, 0x00, 0xd8, 0x03, 0x19, 0x54, 0xf9, 0x08, 0x64, 0xd4, 0x3c, 0x24, 0x7f,
		0x4b, 0xf5, 0xf0, 0x66, 0x5c, 0x6b, 0x50, 0xcc, 0x17, 0x74, 0x9a, 0x27, 0xd1, 0xcf, 0x76, 0x64
	};
	return Crypto::sign(privateKey, messageHash, signature);
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
