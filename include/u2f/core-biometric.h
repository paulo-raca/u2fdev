#pragma once

#include <u2f/core.h>
#include <sqlite3.h>
#include <veridisbiometric.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <chrono>

namespace u2f {
	class BiometricCore : public Core {
		sqlite3 *db;

		std::mutex captureMutex;
		volatile bool isCapturing;
		std::thread *captureThread;
		std::chrono::steady_clock::time_point captureTimeout;
		std::condition_variable captureTimeoutCondition;

		char* fingerprintTemplate;
		int fingerprintTemplateSize;

		static void captureTimeoutThreadFunc(BiometricCore* core);
		void onCaptureEvent(int eventType, const char* readerName, VrBio_BiometricImage* image);
		void enableCapture();
		void captureCompleted(bool join=false);

	public:
		BiometricCore(const char* filename);
		~BiometricCore();

		virtual bool supportsWink();
		virtual void wink();
		virtual bool enroll(const crypto::Hash &applicationHash, Handle &handle, uint8_t &handleSize, crypto::PublicKey &publicKey);
		virtual crypto::Signer* authenticate(const crypto::Hash &applicationHash, const Handle &handle, uint8_t handleSize, bool checkUserPresence, bool &userPresent, uint32_t &authCounter);
		virtual crypto::Signer* getAttestationSigner();
	};
}
