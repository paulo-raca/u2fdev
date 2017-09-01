#include <u2f/hid.h>

#include <stdio.h>
#include <string.h>

#ifdef U2FHID_LOG
#define LOG(fmt, ...) fprintf(stderr, "u2f-hid: " fmt "\n", ##__VA_ARGS__)
static void dump(const char* name, const void* buffer, int length) {
	fprintf(stderr, "u2f-hid: %s: {", name);
	for (int i=0; i<length; i++) {
		if (i % 16 == 0) {
			fprintf(stderr, "\n    ");
		} else {
			fprintf(stderr, " ");
		}

		fprintf(stderr, "%02x", ((uint8_t*)buffer)[i]);
	}
	fprintf(stderr, "\n}\n");
}
#else
#define LOG(fmt, ...)
static inline void dump(const char* name, const void* buffer, int length) {}
#endif


#define min(a,b) (a<b?a:b)
#define max(a,b) (a>b?a:b)

#define REQUEST_PACKET_SIZE 64
#define RESPONSE_PACKET_SIZE 64

#define CID_BROADCAST           0xffffffff

#define CMD_PING                0x81 // Echo data through local processor only
#define CMD_MSG                 0x83 // Send U2F message frame
#define CMD_LOCK                0x84 // Send lock channel command
#define CMD_INIT                0x86 // Channel initialization
#define CMD_WINK                0x88 // Send device identification wink
#define CMD_ERROR               0xbf // Error response

#define ERR_NONE                0x00 // No error
#define ERR_INVALID_CMD         0x01 // Invalid command
#define ERR_INVALID_PAR         0x02 // Invalid parameter
#define ERR_INVALID_LEN         0x03 // Invalid message length
#define ERR_INVALID_SEQ         0x04 // Invalid message sequencing
#define ERR_MSG_TIMEOUT         0x05 // Message has timed out
#define ERR_CHANNEL_BUSY        0x06 // Channel busy
#define ERR_LOCK_REQUIRED       0x0a // Command requires channel lock
#define ERR_INVALID_CID         0x0b // Command not allowed on this cid
#define ERR_OTHER               0x7f // Other unspecified error

#define CAPFLAG_WINK            1	// Device supports WINK command
#define CAPFLAG_LOCK            2	// Device supports LOCK command

#define U2FHID_VERSION          2
#define DEVICE_VERSION_MAJOR    1
#define DEVICE_VERSION_MINOR    0
#define DEVICE_VERSION_BUILD    0

#define MAX_LOCK_TIMEOUT       10


#ifdef __linux__
#include <time.h>

static uint32_t now() {
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);
	return spec.tv_sec * 1000 + spec.tv_nsec / 1000000;
}
#else
#error now() not implemented
#endif

u2f::HidLock::HidLock()
: lockedUntil(now())
{ }

void u2f::HidLock::lock(uint32_t channel, uint8_t seconds) {
	this->channel = channel;
	this->lockedUntil = now() + seconds * 1000;
}
bool u2f::HidLock::isLocked(uint32_t channel) {
	return (channel != this->channel) && (now() < lockedUntil);
}

static const uint8_t descriptor[] = {
    0x06, 0xd0, 0xf1,   // Usage Page (0xf1d0)
    0x09, 0x01,         // Usage (0x01)
    0xa1, 0x01,         // Collection (Application)
    0x09, 0x20,         //   Usage (FIDO Usage Data In)
    0x15, 0x00,         //     Logical Min (0)
    0x26, 0xff, 0x00,   //     Logical Max (255)
    0x75, 0x08,         //     Report Size (8)
    0x95, REQUEST_PACKET_SIZE,  //     Report Count (64)
    0x81, 0x02,         //     Input (Data, Absolute, Variable)
    0x09, 0x21,         //   Usage (FIDO Usage Data Out)
    0x15, 0x00,         //     Logical Min (0)
    0x26, 0xff, 0x00,   //     Logical Max (255)
    0x75, 0x08,         //     Report Size (8)
    0x95, RESPONSE_PACKET_SIZE,  //     Report Count (64)
    0x91, 0x02,         //     Output (Data, Absolute, Variable)
    0xc0                // End Collection
};

void u2f::Hid::getDescriptor(const uint8_t* &descriptorBuffer, uint16_t &descriptorSize) {
	descriptorBuffer = descriptor;
	descriptorSize = sizeof(descriptor);
}

bool u2f::Hid::receivedOutputReport(hiddev::ReportType reportType, uint8_t reportNum, const uint8_t* reportBuffer, uint16_t reportSize) {
	if (reportType != hiddev::ReportType::Output)
		return false;
	if (reportNum != 0)
		return false;

	// I'm currently getting an extra byte. WHY?!
	reportBuffer++;
	reportSize--;
	if (reportSize != REQUEST_PACKET_SIZE)
		return false;

	uint32_t cid = (reportBuffer[0] << 24) | (reportBuffer[1] << 16) | (reportBuffer[2] << 8) | (reportBuffer[3] << 0);
	uint8_t cmd = reportBuffer[4];

	u2f::MultipartHidRequest* pendingMultipartRequest = getPendingMultipartHidRequest(cid);

	//New command
	if (cmd & 0x80) {
		uint16_t payloadSize = (reportBuffer[5] << 8) | (reportBuffer[6] << 0);

		// Cancel pending requests on this channel
		if (pendingMultipartRequest != nullptr)
			pendingMultipartRequest->cancel();


		if (payloadSize <= reportSize - 7) {
			handleRequest(cid, cmd, reportBuffer + 7, payloadSize);
		} else {
			//Start new multipart request
			u2f::MultipartHidRequest* newMultipartRequest = getFreeMultipartHidRequest();
			if (newMultipartRequest == nullptr) {
				LOG("Failed to starting multi-part request -- No free objects");
				sendErrorResponse(cid, ERR_CHANNEL_BUSY);
				return true;
			}
			newMultipartRequest->start(cid, cmd, payloadSize, reportBuffer + 7, reportSize - 7);
		}
	} else {
		//continue multipart request
		if (pendingMultipartRequest == nullptr) {
			LOG("Failed to continue multi-part request -- Mutipart request not found");
			sendErrorResponse(cid, ERR_INVALID_SEQ);
			return true;
		}

		if (!pendingMultipartRequest->append(cmd, reportBuffer + 5, reportSize - 5)) {
			LOG("Failed to continue multi-part request -- Invalid sequencing");
			pendingMultipartRequest->cancel();
			sendErrorResponse(cid, ERR_INVALID_SEQ);
			return true;
		}

		if (pendingMultipartRequest->isComplete()) {
			handleRequest(cid, pendingMultipartRequest->cmd, pendingMultipartRequest->payload, pendingMultipartRequest->payloadSize);
			pendingMultipartRequest->cancel();
		}
	}
	return true;
}

void u2f::Hid::handleRequest(uint32_t cid, uint8_t cmd, const uint8_t* payload, uint16_t payloadSize) {
	LOG("handleRequest: cid=%d, cmd=%d, size=%d", cid, cmd, payloadSize);
	dump("Payload", payload, payloadSize);

	// Fail requests if there is an active lock on other channel
	// I've decided to make exceptions to INIT and PING, which are stateless and can go through :)
	if (lock.isLocked(cid) && (cmd != CMD_INIT) && (cmd != CMD_PING)) {
		LOG("CMD %d on cid=%d failed due to lock from cid=%d :/", cmd, cid, lock.channel);
		sendErrorResponse(cid, ERR_LOCK_REQUIRED);
		return;
	}

	switch (cmd) {
		case CMD_INIT: {
			if (cid != CID_BROADCAST) {
				LOG("CMD_INIT failed: Must use broadcast CID");
				sendErrorResponse(cid, ERR_INVALID_CMD);
				return;
			}
			if (payloadSize != 8) {
				LOG("CMD_INIT failed: Payload must have 8 bytes");
				sendErrorResponse(cid, ERR_INVALID_LEN);
				return;
			}
			uint32_t new_cid = ++channelIdCount;
			uint8_t response[17];
			memcpy(response, payload, 8); // Copy the nonce
			response[ 8] = (uint8_t)(new_cid >> 24);
			response[ 9] = (uint8_t)(new_cid >> 16);
			response[10] = (uint8_t)(new_cid >>  8);
			response[11] = (uint8_t)(new_cid >>  0);
			response[12] = U2FHID_VERSION;
			response[13] = DEVICE_VERSION_MAJOR;
			response[14] = DEVICE_VERSION_MINOR;
			response[15] = DEVICE_VERSION_BUILD;
			response[16] = CAPFLAG_LOCK;
			if (core.supportsWink()) {
				response[16] |= CAPFLAG_WINK;
			}

			LOG("CMD_INIT succeeded: CID=%d", new_cid);
			sendResponse(CID_BROADCAST, CMD_INIT, response, sizeof(response));
			break;
		}
		case CMD_MSG: {
			if (cid == CID_BROADCAST) {
				LOG("CMD_MSG failed: Cannot use broadcast CID");
				sendErrorResponse(cid, ERR_INVALID_CMD);
				return;
			}
			const uint8_t* response = nullptr;
			uint32_t responseSize = 0;
			if (!core.processRawAdpu(payload, payloadSize, response, responseSize)) {
				sendErrorResponse(cid, ERR_INVALID_PAR);
				LOG("CMD_MSG failed: Cannot parse ADPU");
			} else {
				sendResponse(cid, CMD_MSG, response, responseSize);
				LOG("CMD_MSG succeeded");
			}
			if (response)
				delete response;
			break;
		}
		case CMD_PING: {
			sendResponse(cid, CMD_PING, payload, payloadSize);
			LOG("CMD_PING succeeded");
			break;
		}
		case CMD_WINK: {
			if (payloadSize != 0) {
				sendErrorResponse(cid, ERR_INVALID_LEN);
				return;
			}
			core.wink();
			sendResponse(cid, CMD_WINK, nullptr, 0);
			LOG("CMD_WINK succeeded");
			break;
		}
		case CMD_LOCK: {
			if (payloadSize != 1) {
				sendErrorResponse(cid, ERR_INVALID_LEN);
				LOG("CMD_LOCK failed: Payload must have 1 byte");
				return;
			}
			if (payload[0] > MAX_LOCK_TIMEOUT) {
				sendErrorResponse(cid, ERR_INVALID_PAR);
				LOG("CMD_LOCK failed: Timeout must be <= %d, was %d", MAX_LOCK_TIMEOUT, payload[0]);
				return;
			}
			lock.lock(cid, payload[0]);
			sendResponse(cid, CMD_LOCK, nullptr, 0);
			LOG("CMD_LOCK succeeded: %d seconds", payload[0]);
			break;
		}
		default: {
			sendErrorResponse(cid, ERR_INVALID_CMD);
			LOG("Unknown CMD: %d", cmd);
			break;
		}
	}
}

void u2f::Hid::sendResponse(uint32_t cid, uint8_t cmd, const uint8_t* payload, uint16_t payloadSize) {
	dump("Response", payload, payloadSize);
	uint8_t msg[RESPONSE_PACKET_SIZE];
	msg[0] = (uint8_t)(cid >> 24);
	msg[1] = (uint8_t)(cid >> 16);
	msg[2] = (uint8_t)(cid >>  8);
	msg[3] = (uint8_t)(cid >>  0);
	msg[4] = cmd;
	msg[5] = (uint8_t)(payloadSize >>  8);
	msg[6] = (uint8_t)(payloadSize >>  0);

	uint16_t sz = min(payloadSize, RESPONSE_PACKET_SIZE - 7);
	memcpy(msg + 7, payload, sz);
	memset(msg + 7 + sz, 0, RESPONSE_PACKET_SIZE - sz - 7);
	sendInputReport(0, msg, RESPONSE_PACKET_SIZE);

	uint8_t seq = 0;
	payload += sz;
	payloadSize -= sz;
	while (payloadSize) {
		msg[4] = seq++ & 0x7F;  // seq should never be >= 128
		sz = min(payloadSize, RESPONSE_PACKET_SIZE - 5);
		memcpy(msg + 5, payload, sz);
		memset(msg + 5 + sz, 0, RESPONSE_PACKET_SIZE - sz - 5);
		payload += sz;
		payloadSize -= sz;
		sendInputReport(0, msg, RESPONSE_PACKET_SIZE);
	}
}

void u2f::Hid::sendErrorResponse(uint32_t cid, uint8_t err) {
	sendResponse(cid, CMD_ERROR, &err, 1);
}

u2f::MultipartHidRequest* u2f::Hid::getPendingMultipartHidRequest(uint32_t cid) {
	for (int i=0; i<HID_MAX_PENDING_REQUESTS; i++) {
		if (multipartRequest[i].cid == cid && !multipartRequest[i].isExpired()) {
			return &multipartRequest[i];
		}
	}
	return nullptr;
}

u2f::MultipartHidRequest* u2f::Hid::getFreeMultipartHidRequest() {
	for (int i=0; i<HID_MAX_PENDING_REQUESTS; i++) {
		if (multipartRequest[i].isExpired()) {
			return &multipartRequest[i];
		}
	}
	return nullptr;
}

u2f::MultipartHidRequest::MultipartHidRequest() {
	cid = 0;
	expiresAt = now();
	payload = nullptr;
}
u2f::MultipartHidRequest::~MultipartHidRequest() {
	if (payload) {
		delete payload;
		payload = nullptr;
	}
}
void u2f::MultipartHidRequest::cancel() {
	expiresAt = now();
	if (payload) {
		delete payload;
		payload = nullptr;
	}
}

void u2f::MultipartHidRequest::start(uint32_t cid, uint8_t cmd, uint16_t payloadSize, const uint8_t* firstPayload, uint16_t firstPayloadSize) {
	this->cid = cid;
	this->cmd = cmd;
	this->seq = 0;
	this->payloadSize = payloadSize;
	if (this->payload) {
		delete this->payload;
	}
	this->payload = new uint8_t[payloadSize];
	memcpy(this->payload, firstPayload, min(firstPayloadSize, payloadSize));
	this->currentPayloadSize = min(this->payloadSize, firstPayloadSize);
	this->expiresAt = now() + 3000;
}

bool u2f::MultipartHidRequest::append(uint8_t seq, const uint8_t* partialPayload, uint16_t partialPayloadSize) {
	if (seq != (this->seq & 0x7f)) {
		return false;
	}
	uint16_t bytesToCopy = min(partialPayloadSize, this->payloadSize - this->currentPayloadSize);
	memcpy(this->payload + this->currentPayloadSize, partialPayload, bytesToCopy);
	this->currentPayloadSize += bytesToCopy;
	this->seq++;
	return true;
}

bool u2f::MultipartHidRequest::isExpired() {
	return now() >= this->expiresAt;
}

bool u2f::MultipartHidRequest::isComplete() {
	return this->currentPayloadSize == this->payloadSize;
}
