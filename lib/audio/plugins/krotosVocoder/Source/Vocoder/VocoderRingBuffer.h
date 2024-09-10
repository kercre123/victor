#ifndef VOCODERRINGBUFFER_H_INCLUDED
#define VOCODERRINGBUFFER_H_INCLUDED

/*
==============================================================================

VocoderRingBuffer.h
Created: 09 February 2017 12:36:11pm
Author:  Mathieu Carre

==============================================================================
*/

#include <algorithm>  
#include <cstring>
#include <memory>
#include <vector>


using namespace std;

/** Ring buffer helper class
 */
class VocoderRingBuffer
{
public:    
    VocoderRingBuffer(int size);

    int getLength();
    void write(float value);
    float read();
    void read(float* buffer, int numberOfSamplesToRead, int readHeadShift);
    void clear(float value);
    void add(float* buffer, int size, int offset);
    int getNumberOfAvailableSamples();
    float getMedianValue();
    
private:
    int m_length;
    vector<float> m_buffer;
    int m_readHead {0};
    int m_writeHead{0};
};



#endif
