#ifndef WHEEL_CONTROLLER_H
#define WHEEL_CONTROLLER_H

#include "cozmoTypes.h"

void SetDesiredSpeed(s16 speed_mm_per_sec);
s16 GetDesiredSpeed(void);


void ManageWheelSpeedController(s16 fidx);

#endif // WHEEL_CONTROLLER_H