#include <u2f/core.h>
#include <uECC.h>
#include <sha256.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

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


struct uECC_SHA256 {
	uECC_HashContext uECC;
	SHA256_CTX ctx;
	uint8_t tmp[128];

	static void init(const uECC_HashContext *base) {
		uECC_SHA256 *context = (uECC_SHA256 *)base;
		sha256_init(&context->ctx);
	}
	static void update(const uECC_HashContext *base, const uint8_t *message, unsigned message_size) {
		uECC_SHA256 *context = (uECC_SHA256 *)base;
		sha256_update(&context->ctx, message, message_size);
	}
	static void finish(const uECC_HashContext *base, uint8_t *hash_result) {
		uECC_SHA256 *context = (uECC_SHA256 *)base;
		sha256_final(&context->ctx, hash_result);
	}

	uECC_SHA256() {
		uECC.init_hash = &init;
		uECC.update_hash = &update;
		uECC.finish_hash = &finish;
		uECC.block_size = 64;
		uECC.result_size = 32;
		uECC.tmp = tmp;
	}
};













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
			return processRegisterRequest(request, requestSize, response, responseSize);
		}

		case INS_AUTHENTICATE: {
			LOG("INS_AUTHENTICATE -- Not supported yet");
			responseSize = 0;
			return SW_INS_NOT_SUPPORTED;
		}

		case INS_VERSION: {
			LOG("INS_VERSION");
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
	uint8_t &responsePublicKeyType = response[responseSize];
	responseSize++;
	responsePublicKeyType = 0x04;

	uint8_t *responsePublicKey = &response[responseSize];
	responseSize += 64;

	uint8_t privateKey[32];

	if (!uECC_make_key(responsePublicKey, privateKey, uECC_secp256r1())) {
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
	getAttestationCertificate(attestationPrivateKey, attestationCertificateDer, attestationCertificateDerSize);

	memcpy(responseAttestationCertificate, attestationCertificateDer, attestationCertificateDerSize);
	responseSize += attestationCertificateDerSize;

	//Signature
	uint8_t *responseSignature = &response[responseSize];
	uint8_t responseSignatureSize = 0;
	uint8_t hash[32];
	uint8_t signature[64];
	{
		SHA256_CTX hashContext;
		sha256_init(&hashContext);
		uint8_t hash_reserved = 0;
		sha256_update(&hashContext, &hash_reserved, 1);
		sha256_update(&hashContext, applicationHash, 32);
		sha256_update(&hashContext, challengeHash, 32);
		sha256_update(&hashContext, responseKeyHandle, responseKeyHandleSize);
		sha256_update(&hashContext, &responsePublicKeyType, 1);
		sha256_update(&hashContext, responsePublicKey, 64);
		sha256_final(&hashContext, hash);

		uECC_SHA256 eccHash;

		uECC_sign_deterministic(
			attestationPrivateKey,
			hash,
			sizeof(hash),
			&eccHash.uECC,
			signature,
			uECC_secp256r1());

		//Build a fucking DER wrapper
		responseSignature[responseSignatureSize++] = 0x30; //A header byte indicating a compound structure.
		responseSignature[responseSignatureSize++] = 0; // Will fill later


		responseSignature[responseSignatureSize++] = 0x02; //Integer number
		if (signature[0] & 0x80) {
			responseSignature[responseSignatureSize++] = 0x21;
			responseSignature[responseSignatureSize++] = 0x00;
		} else {
			responseSignature[responseSignatureSize++] = 0x20;
		}
		memcpy(&responseSignature[responseSignatureSize], &signature[0], 32);
		responseSignatureSize += 32;


		responseSignature[responseSignatureSize++] = 0x02; //Integer number
		if (signature[32] & 0x80) {
			responseSignature[responseSignatureSize++] = 0x21;
			responseSignature[responseSignatureSize++] = 0x00;
		} else {
			responseSignature[responseSignatureSize++] = 0x20;
		}
		memcpy(&responseSignature[responseSignatureSize], &signature[32], 32);
		responseSignatureSize += 32;

		responseSignature[1] = responseSignatureSize - 2; //Fill Signature size

		responseSize += responseSignatureSize;
	}
	return SW_NO_ERROR;
}



// Not exactly secure, but it is only a test implementation.
void u2f::Core::createHandle(const uint8_t *applicationHash, const uint8_t *privateKey, uint8_t *handle, uint8_t &handleSize) {
	memcpy(handle   , applicationHash, 32);
	memcpy(handle+32, privateKey, 32);
	handleSize = 64;
}

bool u2f::Core::fetchHandle(const uint8_t *applicationHash, uint8_t *handle, uint8_t handleSize, uint8_t *privateKey) {
	if (handleSize != 64)
		return false; // Wrong size
	if (memcmp(applicationHash, handle, 32))
		return false; // Wrong applications

	memcpy(privateKey, handle+32, 32);
	return true;
}
void u2f::Core::getAttestationCertificate(const uint8_t *&privateKey, const uint8_t *&certificate, uint16_t &certificateSize) {
	static const uint8_t savedPrivateKey[] =
		"\xf3\xfc\xcc\x0d\x00\xd8\x03\x19\x54\xf9\x08\x64\xd4\x3c\x24\x7f"
		"\x4b\xf5\xf0\x66\x5c\x6b\x50\xcc\x17\x74\x9a\x27\xd1\xcf\x76\x64";
	static const uint8_t savedCertificate[] =
		"\x30\x82\x01\x3c\x30\x81\xe4\xa0\x03\x02\x01\x02\x02\x0a\x47\x90"
		"\x12\x80\x00\x11\x55\x95\x73\x52\x30\x0a\x06\x08\x2a\x86"
		"\x48\xce\x3d\x04\x03\x02\x30\x17\x31\x15\x30\x13\x06\x03"
		"\x55\x04\x03\x13\x0c\x47\x6e\x75\x62\x62\x79\x20\x50\x69"
		"\x6c\x6f\x74\x30\x1e\x17\x0d\x31\x32\x30\x38\x31\x34\x31"
		"\x38\x32\x39\x33\x32\x5a\x17\x0d\x31\x33\x30\x38\x31\x34"
		"\x31\x38\x32\x39\x33\x32\x5a\x30\x31\x31\x2f\x30\x2d\x06"
		"\x03\x55\x04\x03\x13\x26\x50\x69\x6c\x6f\x74\x47\x6e\x75"
		"\x62\x62\x79\x2d\x30\x2e\x34\x2e\x31\x2d\x34\x37\x39\x30"
		"\x31\x32\x38\x30\x30\x30\x31\x31\x35\x35\x39\x35\x37\x33"
		"\x35\x32\x30\x59\x30\x13\x06\x07\x2a\x86\x48\xce\x3d\x02"
		"\x01\x06\x08\x2a\x86\x48\xce\x3d\x03\x01\x07\x03\x42\x00"
		"\x04\x8d\x61\x7e\x65\xc9\x50\x8e\x64\xbc\xc5\x67\x3a\xc8"
		"\x2a\x67\x99\xda\x3c\x14\x46\x68\x2c\x25\x8c\x46\x3f\xff"
		"\xdf\x58\xdf\xd2\xfa\x3e\x6c\x37\x8b\x53\xd7\x95\xc4\xa4"
		"\xdf\xfb\x41\x99\xed\xd7\x86\x2f\x23\xab\xaf\x02\x03\xb4"
		"\xb8\x91\x1b\xa0\x56\x99\x94\xe1\x01\x30\x0a\x06\x08\x2a"
		"\x86\x48\xce\x3d\x04\x03\x02\x03\x47\x00\x30\x44\x02\x20"
		"\x60\xcd\xb6\x06\x1e\x9c\x22\x26\x2d\x1a\xac\x1d\x96\xd8"
		"\xc7\x08\x29\xb2\x36\x65\x31\xdd\xa2\x68\x83\x2c\xb8\x36"
		"\xbc\xd3\x0d\xfa\x02\x20\x63\x1b\x14\x59\xf0\x9e\x63\x30"
		"\x05\x57\x22\xc8\xd8\x9b\x7f\x48\x88\x3b\x90\x89\xb8\x8d"
		"\x60\xd1\xd9\x79\x59\x02\xb3\x04\x10\xdf";

	privateKey = savedPrivateKey;
	certificate = savedCertificate;
	certificateSize = sizeof(savedCertificate) - 1;
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
