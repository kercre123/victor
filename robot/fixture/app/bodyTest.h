#ifndef BODY_TEST_H
#define BODY_TEST_H

#include "hal/portable.h"

// Return true if device is detected on contacts
bool BodyDetect(void);
  
TestFunction* GetBodyTestFunctions(void);

#endif
