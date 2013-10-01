#ifndef SIM_MOTORS_H
#define SIM_MOTORS_H

#include "cozmoTypes.h"




void SetLiftAngle(float angle);
float GetLiftAngle(void);

//Set an angular wheel velocity in rad/sec
void SetAngularWheelVelocity(float left, float right);


void SetHeadAngle(float pitch_angle);
float GetHeadAngle(void);

#endif // SIM_MOTORS_H
