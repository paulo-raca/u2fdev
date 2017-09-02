#include <u2f/crypto-simple.h>


u2f::crypto::SimpleSigner::SimpleSigner(const u2f::crypto::PrivateKey &privateKey)
: privateKey(privateKey), certificate(nullptr), certificateSize(0)
{}

u2f::crypto::SimpleSigner::SimpleSigner(const u2f::crypto::PrivateKey &privateKey, const uint8_t *certificate, uint16_t certificateSize)
: privateKey(privateKey), certificate(certificate), certificateSize(certificateSize)
{}

bool u2f::crypto::SimpleSigner::sign(const u2f::crypto::Hash &messageHash, u2f::crypto::Signature &signature) {
	return crypto::sign(privateKey, messageHash, signature);
}

bool u2f::crypto::SimpleSigner::getCertificate(const uint8_t *&certificate, uint16_t &certificateSize) {
	certificate = this->certificate;
	certificateSize = this->certificateSize;
	return certificate != nullptr;
}
