#include <stdint.h>

extern "C" {
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
}

#include "bluetoothTask.h"

#include "anki/cozmo/robot/esp.h"
#include "clad/robotInterface/messageEngineToRobot_send_helper.h"

using namespace Anki::Cozmo;

bool Bluetooth::Init(void) {
	// STUB
	return true;
}

void Bluetooth::ConnectionState(uint8_t state) {
	// STUB
}

void Bluetooth::Receive(BLE::Frame& frame) {
	// THIS JUST MIRRORS DATA BACK UP TO THE CENTRAL DEVICE

	BLE::SendData msg;
	memcpy(&msg.frame, &frame, sizeof(BLE::Frame));

	RobotInterface::SendMessage(msg);
}
