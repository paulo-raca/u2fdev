#include <u2f/core-unsafe.h>
#include <string.h>
#include <time.h>

bool u2f::UnsafeCore::createHandle(const crypto::Hash &applicationHash, const crypto::PrivateKey &privateKey, Handle &handle, uint8_t &handleSize) {
	handleSize = sizeof(crypto::Hash) + sizeof(crypto::PrivateKey);
	memcpy(handle, privateKey, sizeof(crypto::PrivateKey));
	memcpy(handle + sizeof(crypto::PrivateKey), applicationHash, sizeof(crypto::Hash));
	return true;
}

bool u2f::UnsafeCore::fetchHandle(const u2f::crypto::Hash &applicationHash, const u2f::Handle &handle, uint8_t handleSize, u2f::crypto::PrivateKey &privateKey, uint32_t &authCounter) {
	// Invalid size
	if (handleSize != sizeof(crypto::Hash) + sizeof(crypto::PrivateKey)) {
		return false;
	}
	// Invalid applicationHash
	if (memcmp(handle + sizeof(crypto::PrivateKey), applicationHash, sizeof(crypto::Hash))) {
		return false;
	}

	// Sounds OK, output the privateKey
	memcpy(privateKey, handle, sizeof(crypto::PrivateKey));

	// AuthCounter must be monotonically increasing.
	// Since we want to be stateless, we can use timestamp for it
	struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
	authCounter = spec.tv_sec;

	return true;
}
