/*
==============================================================================

VocoderRingBuffer.cpp
Created: 09 February 2017 12:36:11pm
Author:  Mathieu Carre

==============================================================================
*/

#include "VocoderRingBuffer.h"
#include <math.h>

VocoderRingBuffer::VocoderRingBuffer(int size) :
    m_buffer(size, 0.0f)
{
    m_length = size;

    m_writeHead = 0;
    m_readHead = 0;
}

int VocoderRingBuffer::getLength()
{
    return m_length;
}

void VocoderRingBuffer::write(float value)
{
    m_buffer[m_writeHead] = value;

    m_writeHead++;

    if (m_writeHead >= m_length)
    {
        m_writeHead = 0;
    }
}

float VocoderRingBuffer::read()
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

void VocoderRingBuffer::clear(float value)
{
    for (int i = 0; i < m_length; i++)
    {
        m_buffer[i] = value;
    }

    m_writeHead = 0;
    m_readHead = 0;
}

void VocoderRingBuffer::read(float* buffer, int numberOfSamplesToRead, int readHeadShift)
{
    if (m_readHead + numberOfSamplesToRead < m_length)
    {
        memcpy(buffer, m_buffer.data() + m_readHead, numberOfSamplesToRead * sizeof(float));
    }
    else
    {
        int size1 = m_length - m_readHead;
        memcpy(buffer, m_buffer.data() + m_readHead, size1 * sizeof(float));

        int size2 = numberOfSamplesToRead - size1;
        memcpy(buffer + size1, m_buffer.data(), size2 * sizeof(float));
    }

    m_readHead += readHeadShift;

    if (m_readHead >= m_length)
    {
        m_readHead -= m_length;
    }
}

void VocoderRingBuffer::add(float* buffer, int size, int offset)
{
    int index = m_writeHead;

    for (int i = 0; i < size; i++)
    {
        m_buffer[index] += buffer[i];

        index++;

        if (index >= m_length)
        {
            index = 0; 
        }
    }

    m_writeHead += offset;

    if (m_writeHead >= m_length)
    {
        m_writeHead -= m_length;
    }
}

int VocoderRingBuffer::getNumberOfAvailableSamples()
{
    int numberOfSamples = m_writeHead - m_readHead;
    
    if (numberOfSamples < 0)
    {
        numberOfSamples += m_length;
    }

    return numberOfSamples;
}

float VocoderRingBuffer::VocoderRingBuffer::getMedianValue()
{
    vector<float> temp(m_buffer);

    std::sort(temp.begin(), temp.end());

    int medianIndex = floor(temp.size() / 2.0f);
    float medianValue = temp[medianIndex];

    return medianValue;
}
