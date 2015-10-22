#ifndef PCB_TEST_H
#define PCB_TEST_H

#include "hal/portable.h"

// Keep sync'd with vehicle project
#define GPIO_TEST_ODR                   (1 << 0)
#define GPIO_TEST_OD                    (1 << 1)

#define GPIO_TEST_MODER_SHIFT           (2)
#define GPIO_TEST_MODER_MASK            (3)
#define GPIO_TEST_MODER_IN              (0)
#define GPIO_TEST_MODER_OUT             (1 << GPIO_TEST_MODER_SHIFT)
#define GPIO_TEST_MODER_AF              (2 << GPIO_TEST_MODER_SHIFT)

#define GPIO_TEST_PUPDR_SHIFT           (4)
#define GPIO_TEST_PUPDR_MASK            (3)
#define GPIO_TEST_PUPDR_NONE            (0)
#define GPIO_TEST_PUPDR_UP              (1 << GPIO_TEST_PUPDR_SHIFT)
#define GPIO_TEST_PUPDR_DOWN            (2 << GPIO_TEST_PUPDR_SHIFT)

#define GPIO_TEST_CHANGE                (1 << 7)

TestFunction* GetPCBTestFunctions(void);

void FlashPCB(void);

#endif
