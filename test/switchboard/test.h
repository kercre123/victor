#pragma once

static bool sTestPassed = true;

#define ASSERT(c, m)  {if(!(c)) {sTestPassed=false;printf("%s\n", m);}}