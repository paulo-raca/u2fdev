/**
 * This exposes one of the built-in cores as UHID device.
 *
 * UHID devices are virtual devices "connected" to the current machine.
 * It's very useful as a stand-alone U2F implementation and for testing purposes,
 *
 * Most of the built-in cores (Except for BiometricCores) do no provide any
 * means of user interaction, which means it considers the users are always present:
 * Registrations and authentications will happen instantly.
 */

#include <u2f/core-unsafe.h>
#include <u2f/core-stateless.h>
#include <u2f/core-biometric.h>
#include <u2f/core-sqlite.h>
#include <u2f/hid.h>
#include <hiddev/uhid.h>

int main() {
	//u2f::UnsafeCore core;
	//u2f::StatelessCore core("Password");
	u2f::SQLiteCore core("handles.db");
	//u2f::BiometricCore core("handles.db");
	u2f::Hid hid(core);
	hiddev::UHid uhid(hid);
	uhid.run();
	return 0;
}
