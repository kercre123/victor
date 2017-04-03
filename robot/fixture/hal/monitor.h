#ifndef MONITOR_H
#define MONITOR_H

#include "hal/portable.h"

// Methods for the INA220 current shunt and power monitor ICs over I2C

// Initialize the monitors and I2C clock
void InitMonitor(void);
void MonitorSetDoubleSpeed(void);

s32 ChargerGetVoltage(void);  //Get the voltage in mV
s32 ChargerGetCurrentMa(void);  //measured current [mA]

s32 BatGetCurrent(void);

#endif
