/**
 * File: boundedWhile
 *
 * Author: damjan
 * Created: 5/7/14
 *
 * Description: helper for bounding while loops
 *
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef UTIL_BOUNDEDWHILE_H_
#define UTIL_BOUNDEDWHILE_H_

namespace AnkiUtil
{


// macro hacking stuff
#define CONCAT_IMPL( x, y ) x##y
#define MACRO_CONCAT( x, y ) CONCAT_IMPL( x, y )

// a while loop that executes a limited number of execution (throws exception)
#define MAKE_NAME MACRO_CONCAT(_bvar_, __LINE__)
//#define BOUNDED_WHILE(n, exp) unsigned int MAKE_NAME=0; while(MAKE_NAME++ > (n) ? throw ::BaseStation::ET_INFINITE_LOOP : (exp))
#define BOUNDED_WHILE(n, exp) unsigned int MAKE_NAME=0; while(MAKE_NAME++ > (n) ? throw std::runtime_error("Bounded while exceeded!") : (exp))



} // end namespace AnkiUtil

#endif //UTIL_BOUNDEDWHILE_H_
