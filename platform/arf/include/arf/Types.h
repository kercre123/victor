#pragma once

#include <mutex>
#include <condition_variable>

#include "util/string/stringUtils.h"

#include <chrono>

// Common types and free functions are defined here

namespace ARF
{

// ============================
// Sync-related classes/methods
// ============================
// TODO C++14
typedef std::mutex                  Mutex;
// typedef std::shared_timed_mutex     Mutex;
typedef std::unique_lock<Mutex>     Lock;
typedef std::unique_lock<Mutex>     WriteLock;
// typedef std::shared_lock<Mutex>     ReadLock; 
typedef Lock ReadLock;
// typedef std::condition_variable_any ConditionVariable;
typedef std::condition_variable ConditionVariable;

// ============================
// UUID-related classes/methods
// ============================

using UUID = std::string;

// ============================
// Time-related classes/methods
// ============================

// Using std::chrono for time for now
// We represent all times as duration since epoch (including negative)
// This makes it easier to serialize/deserialize, but should probably
// be changed in the future to separate Time/Duration concepts

// WallTime are used for wall-time related operations
using WallClock = std::chrono::system_clock;
using WallTime = WallClock::duration;

// SteadyTime is used for ordering events 
using MonotonicClock = std::chrono::steady_clock;
using MonotonicTime = MonotonicClock::duration;

struct TimeParts
{
    int64_t secs;
    int32_t nanosecs;
};

template <typename T>
typename T::duration get_time()
{
    return T::now().time_since_epoch();
}

// Technically 0 just means epoch, but don't have
// a better way to represent invalid time 
template <typename T>
bool time_is_valid( const T& t )
{
    return t != T::zero();
}

template <typename T>
TimeParts time_to_parts( const T& t )
{
    namespace chr = std::chrono;
    auto secs = chr::duration_cast<chr::seconds>( t );
    auto nanosecs = chr::duration_cast<chr::nanoseconds>(t - secs);
    return {secs.count(), static_cast<int32_t>(nanosecs.count())};
}

template <typename T>
T time_from_parts( const TimeParts& p )
{
    namespace chr = std::chrono;
    return chr::duration_cast<T>(chr::seconds( p.secs ) + chr::nanoseconds( p.nanosecs ));
}

template <typename T>
T time_from_parts( int64_t secs, int32_t nanosecs )
{
    namespace chr = std::chrono;
    return chr::duration_cast<T>( chr::seconds( secs ) + chr::nanoseconds( nanosecs ) );
}

template <typename T>
double time_to_float( const T& t )
{
    namespace chr = std::chrono;
    using Seconds = chr::duration<double, chr::seconds::period>;
    return chr::duration_cast<Seconds>( t ).count();
}

template <typename T>
T float_to_time( double t )
{
    namespace chr = std::chrono;
    using Seconds = chr::duration<double, chr::seconds::period>;
    return chr::duration_cast<T>( Seconds( t ) );
}

}
