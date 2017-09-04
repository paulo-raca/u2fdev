#include "hiddev/uhid.h"
#include "u2f/core-unsafe.h"
#include "u2f/core-sqlite.h"
#include "u2f/hid.h"
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <thread>

int main() {
	//u2f::UnsafeCore core;
	u2f::SQLiteCore core("handles.db");
	u2f::Hid hid(core);
	hiddev::UHid uhid(hid);
	uhid.run();
	return 0;
}
