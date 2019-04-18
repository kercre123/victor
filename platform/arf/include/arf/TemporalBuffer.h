#pragma once

#include "arf/Types.h"
#include "util/container/circularBuffer.h"
#include <algorithm>
#include <functional>

namespace ARF
{

// Expect operator< to be implemented for TimeT
template <typename DataT, typename TimeT>
class TemporalBuffer
{
public:

    using Operator = std::function<void(const DataT&)>;

    TemporalBuffer( size_t capacity )
    : _buffer( capacity ) {}

    void Clear()
    {
        WriteLock lock( _mutex );
        _buffer.clear();
    }

    size_t Size() 
    {
        ReadLock lock( _mutex );
        return _buffer.size();
    }

    // Adds the item to the container at the specified time
    void Insert( const TimeT& t, const DataT& d )
    {
        WriteLock lock( _mutex );
        
        // Ghetto in-place bubble sort since there isn't a insert method
        // This might not be terrible since we always add to one side and
        // have bounds on how out-of-order the single item is
        _buffer.push_back( std::make_pair( t, d ) );
        for( int i = _buffer.size() - 1; i > 0; i-- )
        {
            // If the item we added is older than its previous item, swap
            if( _buffer[i].first < _buffer[i-1].first )
            {
                std::swap( _buffer[i], _buffer[i-1] );
            }
            // Otherwise we're done (since all other items are sorted)
            else
            {
                break;
            }
        }
    }

    // Adds all contents to the vector
    void GetAllContents( std::vector<DataT>& data )
    {
        ReadLock lock( _mutex );
        data.reserve( data.size() + _buffer.size() );
        for( int i = 0; i < _buffer.size(); ++i )
        {
            data.push_back( _buffer[i].second );
        }
    }

    // Adds contents bounded by the start/finish times to the vector
    void GetContentsRange( const TimeT& start, const TimeT& finish, std::vector<DataT>& data )
    {
        Operator op = [&data]( const DataT& d )
        {
            data.push_back( d );
        };
        OperateOnRange( start, finish, op );
    }

    // Operates on range of items bounded by the specified start/finish
    void OperateOnRange( const TimeT& start, const TimeT& finish, const Operator& op )
    {
        if( !(start < finish) ) { return; }
        
        ReadLock lock( _mutex );
        if( _buffer.empty() ) { return; }
        if( _buffer.back().first < start ) { return; }

        // TODO Replace this with an efficient binary search
        // NOTE Can't use std::algorithm b/c CircularBuffer doesn't implement iterators
        int i = 1;
        while( i < _buffer.size() )
        {
            if( _buffer[i].first > start ) { break; }
            ++i;
        }
        --i;

        // At this point i points to one element prior to start
        while( i < _buffer.size() )
        {
            if( _buffer[i].first > finish )
            {
                break;
            }
            op( _buffer[i].second );
            i++;
        }
    }

private:

    Mutex _mutex;
    using ItemType = std::pair<TimeT, DataT>;
    using BufferType = Anki::Util::CircularBuffer<ItemType>;
    BufferType _buffer;
};

}