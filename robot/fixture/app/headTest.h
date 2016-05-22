#ifndef PCB_TEST_H
#define PCB_TEST_H

#include "hal/portable.h"

// Return true if device is detected on contacts
bool HeadDetect(void);
TestFunction* GetHeadTestFunctions(void);

#endif
