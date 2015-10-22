#ifndef MONITOR_H
#define MONITOR_H

#include "hal/portable.h"

// Methods for the INA220 current shunt and power monitor ICs over I2C

// Initialize the monitors and I2C clock
void InitMonitor(void);

// Get the current in uA
s32 MonitorGetCurrent(void);

// Get the voltage in mV
s32 MonitorGetVoltage(void);

#endif
