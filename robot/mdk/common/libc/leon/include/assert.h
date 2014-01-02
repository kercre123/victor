#ifndef _ASSERT_H_
#define _ASSERT_H_

// Lightweight assert can be used in cases where debug build is growing too large due to assert strings 
// You can use the debugger to work out where you are as it triggers a breakpoint 
//#define LIGHTWEIGHT_ASSERT  

#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG
#   ifdef LIGHTWEIGHT_ASSERT  
#       define assert(x) {if((x)==0) {printf( "\nAssertion failed @ line %u",__LINE__);asm("TA 1");}}
#   else
#       define assert(x) {if((x)==0) {printf( "\nAssertion failed: \"%s\"\n  on line %u of file %s\n", #x, __LINE__, __FILE__ );exit( -1 );}}
#   endif
#else //exit not defined in stdlib.h and assert does no test on x
  #define assert(x) 
#endif

#endif // _ASSERT_H_
