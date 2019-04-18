#pragma once

#include "arf/Types.h"

namespace ARF
{

// Non-blocking synchronization primitive that tells threads if
// they are within the first number to reach a point in logic
// This is useful for synchronizing multiple tasks without blocking
// the threads (ie Barrier)
class Turnstile
{
public:

    Turnstile() = default;

    // Initializes the traversal check threshold
    // The nth and later calls to Traverse will return false
    void SetThreshold( int n );
    
    // Indicate a thread passing a point in logic
    // Returns true if the thread is in the first N-1 (passing through)
    // or false if it is the Nth or greater (stopped)
    bool Traverse();

private:

    Mutex _mutex;
    int _counter = 0;
};

}