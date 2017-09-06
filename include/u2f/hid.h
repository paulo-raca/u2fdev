#pragma once

#include <u2f/core.h>
#include <hiddev/core.h>

// FIDO U2F HID Protocol
// (Abstract and implementation-agnostic)


#define HID_MAX_PENDING_REQUESTS 10

namespace u2f {

	class MultipartHidRequest {
		uint8_t  seq;
		uint16_t currentPayloadSize;
		uint32_t expiresAt;
	public:
		uint32_t cid;
		uint8_t  cmd;
		uint8_t* payload;
		uint16_t payloadSize;

		MultipartHidRequest();
		~MultipartHidRequest();

		void cancel();
		void start(uint32_t cid, uint8_t cmd, uint16_t payloadSize, const uint8_t* firstPayload, uint16_t firstPayloadSize);
		bool append(uint8_t seq, const uint8_t* partialPayload, uint16_t partialPayloadSize);
		bool isExpired();
		bool isComplete();
	};

	class HidLock {
	public:
		uint32_t channel;
		uint32_t lockedUntil;

		HidLock();

		void lock(uint32_t channel, uint8_t seconds);
		bool isLocked(uint32_t channel);
	};

	class Hid : public u2f::Protocol, public hiddev::Device {
		uint32_t channelIdCount;
		HidLock lock;
		MultipartHidRequest multipartRequest[HID_MAX_PENDING_REQUESTS];

	public:
		inline Hid(Core& core)
			: Protocol(core)
		{ }

		// HID functions
		void getDescriptor(const uint8_t* &descriptorBuffer, uint16_t &descriptorSize) override;
		bool isNumberedReport(hiddev::ReportType reportType) override;
		uint16_t getReportSize(hiddev::ReportType reportType, uint8_t reportNum) override;
		bool receivedOutputReport(hiddev::ReportType reportType, uint8_t reportNum, const uint8_t* reportBuffer, uint16_t reportSize) override;

		// U2F Protocol functions
		void handleRequest(uint32_t cid, uint8_t cmd, const uint8_t* payload, uint16_t payloadSize);
		void sendResponse(uint32_t cid, uint8_t cmd, const uint8_t* payload, uint16_t payloadSize);
		void sendErrorResponse(uint32_t cid, uint8_t err);

		/**
		 * Returns a non-expired MultipartHidRequest which belongs to the specified cid
		 */
		MultipartHidRequest* getPendingMultipartHidRequest(uint32_t cid);
		MultipartHidRequest* getFreeMultipartHidRequest();

	};
}
