#ifndef KEYBOARD_CONTROLLER_H
#define KEYBOARD_CONTROLLER_H

#include "anki/cozmo/robot/cozmoTypes.h"

void EnableKeyboardController(void);
void DisableKeyboardController(void);
BOOL IsKeyboardControllerEnabled(void);
void RunKeyboardController(void);


#endif //KEYBOARD_CONTROLLER_H