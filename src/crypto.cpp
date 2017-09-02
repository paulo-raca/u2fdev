#include <u2f/core.h>

#include <stdarg.h>
#include <uECC.h>
#include <sha256.h>
#include <string.h>

/**
 * SHA256 adapter for uECC
 */
namespace u2f {
	namespace crypto {
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
	}
}



void u2f::crypto::sha256(Hash &hash, ...) {
	va_list ap;
	va_start(ap, hash);

	SHA256_CTX hashContext;
	sha256_init(&hashContext);

	while (true) {
		uint8_t* buffer = va_arg(ap, uint8_t*);
		if (buffer == nullptr) break;
		int bufferSize = va_arg(ap, int);
		sha256_update(&hashContext, buffer, bufferSize);
	}

	sha256_final(&hashContext, hash);
}


bool u2f::crypto::makeKeyPair(PublicKey &publicKey, PrivateKey &privateKey) {
	publicKey[0] = 0x04;  // Curve name: P-256
	return uECC_make_key(publicKey + 1, privateKey, uECC_secp256r1());
}


bool u2f::crypto::sign(const u2f::crypto::PrivateKey &privateKey, const u2f::crypto::Hash &messageHash, u2f::crypto::Signature &signature) {
	uECC_SHA256 eccHash;
	uint8_t rawSignature[64]; //Maybe use signature and perform DER bullshit in-place?

	int sign_success = uECC_sign_deterministic(
		privateKey,
		messageHash,
		sizeof(u2f::crypto::Hash),
		&eccHash.uECC,
		rawSignature,
		uECC_secp256r1());

	if (!sign_success)
		return false;

	uint8_t signatureSize = 0;
	//Wraps the key in a fucking DER container
	signature[signatureSize++] = 0x30; //A header byte indicating a compound structure.
	signature[signatureSize++] = 0; // Will fill later


	signature[signatureSize++] = 0x02; //Integer number
	if (rawSignature[0] & 0x80) {
		signature[signatureSize++] = 0x21;
		signature[signatureSize++] = 0x00;
	} else {
		signature[signatureSize++] = 0x20;
	}
	memcpy(&signature[signatureSize], &rawSignature[0], 32);
	signatureSize += 32;


	signature[signatureSize++] = 0x02; //Integer number
	if (rawSignature[32] & 0x80) {
		signature[signatureSize++] = 0x21;
		signature[signatureSize++] = 0x00;
	} else {
		signature[signatureSize++] = 0x20;
	}
	memcpy(&signature[signatureSize], &rawSignature[32], 32);
	signatureSize += 32;

	signature[1] = signatureSize - 2; //Fill Signature size
	return true;
}


uint8_t u2f::crypto::signatureSize(const u2f::crypto::Signature &signature) {
	return signature[1] + 2;
}

