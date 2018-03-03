
#include "stdlib.h"


#include "cozmoAnim/doom/i_system.h"

#include "cozmoAnim/doom/m_fixed.h"

#include <limits>


// Fixme. __USE_C_FIXED__ or something.

int
FixedMul
( int	a,
  int	b )
{
  return static_cast<int>( ((long long) a * (long long) b) >> FRACBITS );
}



//
// FixedDiv, C version.
//

int
FixedDiv
( int	a,
  int	b )
{
    if ( (abs(a)>>14) >= abs(b))
	return (a^b)<0 ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();
    return FixedDiv2 (a,b);
}



int
FixedDiv2
( int	a,
  int	b )
{
#if 0
    long long c;
    c = ((long long)a<<16) / ((long long)b);
    return (int) c;
#endif

    double c;

    c = ((double)a) / ((double)b) * FRACUNIT;

    if (c >= 2147483648.0 || c < -2147483648.0)
	I_Error("FixedDiv: divide by zero");
    return (int) c;
}
