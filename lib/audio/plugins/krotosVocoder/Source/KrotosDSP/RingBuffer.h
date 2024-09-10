
#ifndef RINGBUFFER_H_INCLUDED
#define RINGBUFFER_H_INCLUDED

/*
==============================================================================

RingBuffer.h
Created: 13 Jan 2017 3:19:35pm
Author:  KrotosMacMini

==============================================================================
*/


#include <cstring>
#include <memory>
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>

/** Ring buffer helper class
 */
class RingBuffer
{
public:    
    RingBuffer(int size, AK::IAkPluginMemAlloc* Allocator);
    ~RingBuffer();
    int getLength();
    void write(float value);
    float read();
    void clear(float value);
    int getNumberOfAvailableSamples();
    float* getBufferPointer();
    
private:
    int m_length;
    float* m_buffer{NULL};
    int m_readHead {0};
    int m_writeHead{0};
    
    AK::IAkPluginMemAlloc* m_allocator{NULL};
};



#endif  // RINGBUFFER_H_INCLUDED
