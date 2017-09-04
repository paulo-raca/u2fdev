#pragma once

#include <u2f/core.h>

namespace u2f {

	/**
	 * Most simple U2F Core cores only need to:
	 * - Check whenever user is present
	 * - Create a Handle from an applicationHash + PrivateKey
	 * - Map an applicationHash + Handle back to a PrivateKey
	 *
	 * If you are doing something fancy, like Hardware encryption, biometric, etc, SimpleCore will not
	 * be adequate for you, but for everybody else it is a fine base class.
	 */
	class SimpleCore : public Core {
		/**
		 * Checks for user presence.
		 *
		 * Ideally, once user presence is detected, U2F tokens should persist the user presence
		 * state for 10 seconds or until an operation which requires user presence is performed,
		 * whichever comes first.
		 *
		 * The default implementation assumes the user is always present, override this as needed
		 *
		 * @return true is the user is present
		 */
		virtual bool isUserPresent();

		/**
		 * Creates a new handle, which should be mapped to the specified privateKey when accessed by the specified applicationHash.
		 *
		 * @param[in] applicationHash Hash of the application the will own the new key
		 * @param[in] privateKey Private Key that will be mapped by the new handle
		 * @param[out] handle The new handle that will be created
		 * @param[out] handleSize Size of #handle
		 *
		 * @return true if a new handle was created successfully.
		 */
		virtual bool createHandle(const crypto::Hash &applicationHash, const crypto::PrivateKey &privateKey, Handle &handle, uint8_t &handleSize) = 0;

		/**
		 * Fetches the private key associated with an applicationHash + Handle.
		 *
		 * @param[in] applicationHash Hash of the application which owns the key
		 * @param[in] handle The handle used to identify the private key
		 * @param[in] handleSize Size of #handle
		 * @param[out] privateKey The privateKey associated with the applicationHash + Handle
		 * @param[out] authCounter Monotonic counter of how many time this handle has been fetched.
		 */
		virtual bool fetchHandle(const crypto::Hash &applicationHash, const Handle &handle, uint8_t handleSize, crypto::PrivateKey &privateKey, uint32_t &authCounter) = 0;


		virtual bool enroll(const crypto::Hash &applicationHash, Handle &handle, uint8_t &handleSize, crypto::PublicKey &publicKey);
		virtual crypto::Signer* authenticate(const crypto::Hash &applicationHash, const Handle &handle, uint8_t handleSize, bool checkUserPresence, bool &userPresent, uint32_t &authCounter);
		virtual crypto::Signer* getAttestationSigner();
	};
}
