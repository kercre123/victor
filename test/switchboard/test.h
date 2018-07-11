#pragma once

#include "stdint.h"

enum MessageState : uint8_t {
  TestRaw,
  TestClad,
  TestSecure,
};

static bool sTestPassed = true;

#define ASSERT(c, m)  {if(!(c)) {sTestPassed=false;printf("%s\n", m);}}