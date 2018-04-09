#ifndef ACCEL_H_
#define ACCEL_H_

#include <stdbool.h>
#include <stdint.h>

//-----------------------------------------------------------
//        Defines
//-----------------------------------------------------------
//empty

//-----------------------------------------------------------
//        Interface
//-----------------------------------------------------------

void accel_power_on(void);
void accel_power_off(void);
bool accel_power_is_on(void);

bool accel_chipinit(void); //true=successful
int  accel_read(void);

#endif //ACCEL_H_

