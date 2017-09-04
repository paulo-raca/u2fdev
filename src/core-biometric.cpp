#include <u2f/core-biometric.h>
#include <u2f/crypto-simple.h>
#include <string.h>
#include <stdio.h>
using namespace std::chrono_literals;

#define LOG(fmt, ...) fprintf(stderr, "u2f-core-biometric: " fmt "\n", ##__VA_ARGS__)

u2f::BiometricCore::BiometricCore(const char* filename) {
	// Open the DB
	int ret = sqlite3_open(filename, &db);
	if (ret != SQLITE_OK) {
		LOG("Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		db = nullptr;
	}

	// Setup the table
	ret = sqlite3_exec(db,
			"CREATE TABLE IF NOT EXISTS Handle ("
			"	applicationHash BLOB,"
			"	handle BLOB,"
			"	privateKey BLOB,"
			"	fingerprintTemplate BLOB,"
			"	authCounter INTEGER DEFAULT 0,"
			"	PRIMARY KEY (applicationHash, handle)"
			");",
			nullptr, nullptr, nullptr);

	if (ret != SQLITE_OK) {
		LOG("Can'create table Handle: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		db = nullptr;
	}

	fingerprintTemplate = nullptr;
	isCapturing = false;
	captureThread = nullptr;
}

u2f::BiometricCore::~BiometricCore() {
	{
		std::unique_lock<std::mutex> lck(captureMutex);
		captureCompleted(true);
	}

	if (db) {
		sqlite3_close(db);
	}
}

void u2f::BiometricCore::onCaptureEvent(int eventType, const char* readerName, VrBio_BiometricImage* image) {
	if (eventType & VRBIO_CAPTURE_EVENT_PLUG) {
		veridiscap_addListenerToReader(this, readerName);
	}

	//Fingerprint removed -- Throw the template away
	if (eventType & (VRBIO_CAPTURE_EVENT_REMOVED | VRBIO_CAPTURE_EVENT_UNPLUG)) {
		veridisutil_templateFree(&fingerprintTemplate);
	}

	//Fingerprint added -- Extract template
	if (eventType & VRBIO_CAPTURE_EVENT_IMAGE_CAPTURED) {
		std::unique_lock<std::mutex> lck(captureMutex);
		if (fingerprintTemplate) {
			veridisutil_templateFree(&fingerprintTemplate);
		}
		veridisbio_extractEx(image, &fingerprintTemplate, &fingerprintTemplateSize, "ISO", nullptr);
	}
}

void u2f::BiometricCore::captureTimeoutThreadFunc(u2f::BiometricCore *core) {
	LOG("captureTimeoutThreadFunc");
	//Start capture
	int addListenerRet = veridiscap_addListener(core, [](int eventType, const char* readerName, VrBio_BiometricImage* image, const void* core){((u2f::BiometricCore*)core)->onCaptureEvent(eventType, readerName, image);});
	LOG("veridiscap_addListener %d", addListenerRet);

	//Wait for completion or Timeout
	{
		std::unique_lock<std::mutex> lck(core->captureMutex);
		while (std::chrono::steady_clock::now() < core->captureTimeout) {
			core->captureTimeoutCondition.wait_until(lck, core->captureTimeout);
		}
	}

	//End capture

	LOG("Ending capture");
	veridiscap_removeListener(core);
	veridisutil_templateFree(&core->fingerprintTemplate);
	core->isCapturing = false;
}

void u2f::BiometricCore::enableCapture() {
	captureTimeout = std::chrono::steady_clock::now() + 5000ms;
	if (!isCapturing) {
		LOG("Initiating capture");
		if (captureThread) { //Delete old threads
			captureThread->join();
			delete captureThread;
			captureThread = nullptr;
		}
		isCapturing = true;
		captureThread = new std::thread(BiometricCore::captureTimeoutThreadFunc, this);
	}
}

void u2f::BiometricCore::captureCompleted(bool join) {
	if (isCapturing) {
		LOG("Capture Successfull");
		//Wait for the thread to die
		captureTimeout = std::chrono::steady_clock::now() - 1000ms;
		captureTimeoutCondition.notify_all();
	}

	if (join && captureThread) {
		captureThread->join();
		delete captureThread;
		captureThread = nullptr;
	}
}

bool u2f::BiometricCore::supportsWink() {
	std::unique_lock<std::mutex> lck(captureMutex);
	return true;
}

void u2f::BiometricCore::wink() {
	if (!db)
		return; // Database is closed, everything is dead

	//Turn on fingerprint scanners
	std::unique_lock<std::mutex> lck(captureMutex);
	enableCapture();
}

bool u2f::BiometricCore::enroll(const u2f::crypto::Hash &applicationHash, u2f::Handle &handle, uint8_t &handleSize, u2f::crypto::PublicKey &publicKey) {
	if (!db)
		return false; // Database is closed

	std::unique_lock<std::mutex> lck(captureMutex);
	enableCapture(); // Scanner must be on
	if (fingerprintTemplate == nullptr) {
		return false;
	}

	//Create the keypair
	crypto::PrivateKey privateKey;
	if (!crypto::makeKeyPair(publicKey, privateKey)) {
		//Failed to create a key.
		//I guess this shoudn't happen?
		return false;
	}

	//Create a new random handle
	handleSize = 64;
	sqlite3_randomness(handleSize, handle);


	sqlite3_stmt *stmt = nullptr;
	int ret = sqlite3_prepare_v2(db, "INSERT INTO Handle (applicationHash, handle, privateKey, fingerprintTemplate) VALUES (?1, ?2, ?3, ?4);", -1, &stmt, nullptr);
	if (ret != SQLITE_OK) {
		LOG("Failed prepare Insert statement: %s\n", sqlite3_errmsg(db));
		return false;
	}
	sqlite3_bind_blob(stmt, 1, applicationHash, sizeof(crypto::Hash), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, handle, handleSize, SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 3, privateKey, sizeof(crypto::PrivateKey), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 4, fingerprintTemplate, fingerprintTemplateSize, SQLITE_STATIC);

	ret = sqlite3_step(stmt);
	sqlite3_finalize(stmt);

	captureCompleted(); // Turn off fingerprint scanner

	if (ret != SQLITE_DONE) {
		LOG("Failed to insert handle: %s\n", sqlite3_errmsg(db));
		return false;
	}
	return true;

}

u2f::crypto::Signer* u2f::BiometricCore::authenticate(const u2f::crypto::Hash &applicationHash, const u2f::Handle &handle, uint8_t handleSize, bool checkUserPresence, bool &userPresent, uint32_t &authCounter) {
	if (!db)
		return nullptr; // Database is closed

	sqlite3_stmt *stmt = nullptr;
	int ret = sqlite3_prepare_v2(db, "Select privateKey, authCounter, fingerprintTemplate FROM Handle WHERE applicationHash = ?1 AND handle = ?2;", -1, &stmt, nullptr);
	if (ret != SQLITE_OK) {
		LOG("Failed prepare 'select handle' statement: %s\n", sqlite3_errmsg(db));
		return nullptr;
	}
	sqlite3_bind_blob(stmt, 1, applicationHash, sizeof(crypto::Hash), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, handle, handleSize, SQLITE_STATIC);

	ret = sqlite3_step(stmt);
	if (ret == SQLITE_DONE) {
		// Handle not found ¯\_(ツ)_/¯
		sqlite3_finalize(stmt);
		return nullptr;
	} else if (ret != SQLITE_ROW) {
		//Some error?!
		LOG("Failed select handle: %s\n", sqlite3_errmsg(db));
		sqlite3_finalize(stmt);
		return nullptr;
	}

	// Fetch privateKey and authCounter
	crypto::PrivateKey privateKey;
	memcpy(privateKey, sqlite3_column_blob(stmt, 0), sizeof(crypto::PrivateKey));
	authCounter = sqlite3_column_int(stmt, 1);

	// Check user presence
	if (checkUserPresence) {
		std::unique_lock<std::mutex> lck(captureMutex);
		enableCapture(); // Scanner must be on
		if (fingerprintTemplate == nullptr) {
			userPresent = false;
		} else {
			char* storedTemplate = (char*)sqlite3_column_blob(stmt, 2);
			int storedTemplateSize = sqlite3_column_bytes(stmt, 2);

			int score = veridisbio_match(storedTemplate, storedTemplateSize, fingerprintTemplate, fingerprintTemplateSize);
			if (score < 0) {
				userPresent = false;
				LOG("Failed to perform fingerprint matching: %d", score);
			} else if (score < 30) {
				userPresent = false;
				LOG("Fingerprints don't match");
			} else {
				//Templates match, user is present
				userPresent = true;
				captureCompleted(); // Turn off fingerprint scanner
			}
		}
	}

	sqlite3_finalize(stmt);

	// Increment authCounter
	ret = sqlite3_prepare_v2(db, "UPDATE Handle SET authCounter = authCounter + 1 WHERE applicationHash = ?1 AND handle = ?2;", -1, &stmt, nullptr);
	if (ret != SQLITE_OK) {
		LOG("Failed prepare 'update authCounter' statement: %s\n", sqlite3_errmsg(db));
		return nullptr;
	}
	sqlite3_bind_blob(stmt, 1, applicationHash, sizeof(crypto::Hash), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, handle, handleSize, SQLITE_STATIC);
	ret = sqlite3_step(stmt);
	if (ret != SQLITE_DONE) {
		LOG("Failed to update authCounter: %s\n", sqlite3_errmsg(db));
		return nullptr;
	}

	return new crypto::SimpleSigner(privateKey);
}

u2f::crypto::Signer* u2f::BiometricCore::getAttestationSigner()  {
	static const crypto::PrivateKey privateKey = {
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
	return new crypto::SimpleSigner(privateKey, certificate, sizeof(certificate));
}
