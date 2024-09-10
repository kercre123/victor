/*
  ==============================================================================

    RingBuffer.cpp
    Created: 13 Jan 2017 3:19:35pm
    Author:  KrotosMacMini

  ==============================================================================
*/

#include "RingBuffer.h"


RingBuffer::RingBuffer(int size, AK::IAkPluginMemAlloc* Allocator)

{
    m_buffer = (float*)AK_PLUGIN_ALLOC(Allocator, sizeof(float)*size);
    
    m_length = size;

    m_writeHead = 0;
    m_readHead = 0;

	m_allocator = Allocator;
}

RingBuffer::~RingBuffer()
{
    if(m_buffer != NULL)
    {
        AK_PLUGIN_FREE(m_allocator, m_buffer);
        m_buffer = NULL;
    }
}

int RingBuffer::getLength()
{
    return m_length;
}

void RingBuffer::write(float value)
{
    m_buffer[m_writeHead] = value;

    m_writeHead++;

    if (m_writeHead >= m_length)
    {
        m_writeHead = 0;
    }
}

float RingBuffer::read()
{
    float value = m_buffer[m_readHead];

    m_buffer[m_readHead] = 0.0f;

    m_readHead++;

    if (m_readHead >= m_length)
    {
        m_readHead = 0;
    }

    return value;
}

void RingBuffer::clear(float value)
{
    for (int i = 0; i < m_length; i++)
    {
        m_buffer[i] = value;
    }

    m_writeHead = 0;
    m_readHead = 0;
}


int RingBuffer::getNumberOfAvailableSamples()
{
    int numberOfSamples = m_writeHead - m_readHead;
    
    if (numberOfSamples < 0)
    {
        numberOfSamples += m_length;
    }

    return numberOfSamples;
}

float* RingBuffer::getBufferPointer()
{
    return m_buffer;
}
