#pragma once

#include "arf/Types.h"
#include <string>
#include <cxxabi.h>
#include <iostream>

namespace ARF
{

// Methods for getting demangled string names of types
// This should work for both GCC and Clang
template <typename T>
std::string get_type_name()
{
    const std::type_info& ti = typeid( T );
    char* realname = abi::__cxa_demangle( ti.name(), 0, 0, 0 );
    std::string out( realname );
    free( realname );
    return out;
}

std::ostream& operator<<( std::ostream& os, const MonotonicTime& t );
std::ostream& operator<<( std::ostream& os, const WallTime& t );

} // end namespace ARF
