#include "arf/TypeHelpers.h"

namespace ARF
{

std::ostream& operator<<( std::ostream& os, const MonotonicTime& t )
{
    TimeParts parts = time_to_parts( t );
    os << parts.secs << "." << parts.nanosecs;
    return os;
}

// TODO MonotonicTime and WallTime both alias to the same thing
// std::ostream& operator<<( std::ostream& os, const WallTime& t )
// {
//     TimeParts parts = time_to_parts( t );
//     os << "W:" << parts.secs << "." << parts.nanosecs;
//     return os;
// }

} // end namespace ARF
