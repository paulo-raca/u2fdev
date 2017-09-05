#include <u2f/core-unsafe.h>
#include <u2f/core-stateless.h>
#include <u2f/core-biometric.h>
#include <u2f/core-sqlite.h>
#include <u2f/hid.h>
#include <hiddev/uhid.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <thread>

int main() {
	//u2f::UnsafeCore core;
	//u2f::SQLiteCore core("handles.db");
	//u2f::StatelessCore core("Password");
	u2f::BiometricCore core("handles.db");
	u2f::Hid hid(core);
	hiddev::UHid uhid(hid);
	uhid.run();
	return 0;
}
