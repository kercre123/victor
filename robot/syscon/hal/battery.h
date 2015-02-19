#ifndef BATTERY_H
#define BATTERY_H

// Initialize the charge pins and sensing
void BatteryInit();

// Update the state of the battery
void BatteryUpdate();

// Turn on power to the head
void PowerOn();

#endif
