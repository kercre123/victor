#include "arf/Turnstile.h"

namespace ARF
{

void Turnstile::SetThreshold( int n )
{
    Lock lock( _mutex );
    _counter = n;
}

bool Turnstile::Traverse()
{
    Lock lock( _mutex );
    _counter--;
    return _counter > 0;
}

} // end namespace ARF