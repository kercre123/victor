// Same purpose as main.cpp, but works with the Keil simulator for benchmarking

#include "anki/cozmo/robot/hal.h"
#include "anki/cozmo/robot/cozmoBot.h"
#include "hal/portable.h"

#include <stdio.h>

extern "C" {
	unsigned int XXX_HACK_FOR_PETE(void)
	{
		return 0;
	}
}

int main(void)
{
	//printf("hello whirrled\n");
	
	Anki::Cozmo::Robot::Init();
	
	return 0;
}
