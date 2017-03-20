#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

#include <stdint.h>
#include "clad/robotInterface/bleMessages.h"

namespace Bluetooth {
	bool Init(void);
	void Receive(Anki::Cozmo::BLE::Frame&);
	void ConnectionState(uint8_t state);
}

#endif
