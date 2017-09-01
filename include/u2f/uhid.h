#include "hid"
#include <linux/uhid.h>

// FIDO U2F HID Protocol
// Implemented as a virtual device using Linux's UHID driver

namespace u2f {

	class UHid : public u2f::Hid {
	public:
		const int fd;

		UHid(Core& core);
		~UHid(Core& core);

		virtual struct uhid_create2_req create();

	protected:
		virtual void send_raw(void* raw, int len);
		virtual void raw_received(void* raw, int len);
	};

}
