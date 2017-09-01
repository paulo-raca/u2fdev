#pragma once

#include <inttypes.h>

namespace u2f {

	class Core {
	public:
		virtual bool processRawAdpu(const uint8_t* rawRequest, uint32_t rawRequestSize, const uint8_t *&rawResponse, uint32_t& rawResponseSize);
		uint16_t processRequest(uint8_t cla, uint8_t ins, uint8_t p1, uint8_t p2, const uint8_t *request, uint32_t requestSize, uint8_t *response, uint32_t &responseSize);
		uint16_t processRegisterRequest(const uint8_t *request, uint32_t requestSize, uint8_t *response, uint32_t &responseSize);
		virtual void wink();
		virtual bool supportsWink();
		virtual bool isUserPresent();

		//For a given applicationHash, register a privateKey and returns a handle
		virtual void createHandle(const uint8_t *applicationHash, const uint8_t *privateKey, uint8_t *handle, uint8_t &handleSize);
		//For a given applicationHash and handle, fetch the privateKey. Returns false if the handle is invalid
		virtual bool fetchHandle(const uint8_t *applicationHash, uint8_t *handle, uint8_t handleSize, uint8_t *privateKey);

		virtual void getAttestationCertificate(const uint8_t *&privateKey, const uint8_t *&certificate, uint16_t &certificateSize);
	};

	class Protocol {
	protected:
		Core& core;

	public:
		inline Protocol(Core& core)
			: core(core)
		{ }
	};

}