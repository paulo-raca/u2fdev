#include <u2f/core-stateless.h>
#include <aes.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#define LOG(fmt, ...) fprintf(stderr, "u2f-core-stateless: " fmt "\n", ##__VA_ARGS__)

u2f::StatelessCore::StatelessCore(const char* password) {
	crypto::Hash passwordHash;
	const char* salt = "U2F Device Library";
	crypto::sha256(
		passwordHash,
		salt, strlen(salt),
		password, strlen(password),
		nullptr);

	aes_key_setup(passwordHash, aesKey, 256);
}

bool u2f::StatelessCore::createHandle(const crypto::Hash &applicationHash, const crypto::PrivateKey &privateKey, Handle &handle, uint8_t &handleSize) {
	// Creates the unencrypted handle: [privateKey, applicationHash]
	handleSize = sizeof(crypto::Hash) + sizeof(crypto::PrivateKey);
	uint8_t rawHandle[sizeof(crypto::PrivateKey) + sizeof(crypto::Hash)];
	memcpy(rawHandle, applicationHash, sizeof(crypto::Hash));
	memcpy(rawHandle + sizeof(crypto::Hash), privateKey, sizeof(crypto::PrivateKey));

	aes_encrypt_cbc(rawHandle, handleSize, handle, aesKey, 256, applicationHash);
	return true;
}

bool u2f::StatelessCore::fetchHandle(const u2f::crypto::Hash &applicationHash, const u2f::Handle &handle, uint8_t handleSize, u2f::crypto::PrivateKey &privateKey, uint32_t &authCounter) {
	// Invalid size
	if (handleSize != sizeof(crypto::Hash) + sizeof(crypto::PrivateKey)) {
		LOG("Invalid handle size: %d", handleSize);
		return false;
	}

	//Decrypt the handle
	uint8_t rawHandle[sizeof(crypto::PrivateKey) + sizeof(crypto::Hash)];
	aes_decrypt_cbc(handle, handleSize, rawHandle, aesKey, 256, applicationHash);

	// Invalid applicationHash
	if (memcmp(rawHandle, applicationHash, sizeof(crypto::Hash))) {
		LOG("applicationHash check failed");
		return false;
	}

	// Sounds OK, output the privateKey
	memcpy(privateKey, rawHandle + sizeof(crypto::Hash), sizeof(crypto::PrivateKey));

	// AuthCounter must be monotonically increasign.
	// Since this is the dumbest possible Core example, we can use timestamp as AuthCounter
	struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
	authCounter = spec.tv_sec;

	return true;
}
