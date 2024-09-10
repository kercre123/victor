/*
==============================================================================

Windows.cpp
Created: 09 February 2017 12:40:33pm
Author:  Mathieu Carre

==============================================================================
*/

#ifndef WINDOW_H_INCLUDED
#define WINDOW_H_INCLUDED

#ifndef ORBIS
#define M_PI 3.141592653589793
#endif

template <typename Sample> struct Window
{
    enum Type
    {
        Rectangular = 0,
        Triangle,
        Hann,
        HannSquareRoot,
        numberOfWindows
    };

    static void generateWindow(Sample* buffer, int bufferSize, Type windowType)
    {
        switch (windowType)
        {
        case Rectangular:
            rectangular(buffer, bufferSize);
            break;

        case Triangle:
            triangular(buffer, bufferSize);
            break;

        case Hann:
            hann(buffer, bufferSize);
            break;

        case HannSquareRoot:
            hannSquareRoot(buffer, bufferSize);
            break;
        case numberOfWindows:
            // DO NOTHING
            break;
        }
    }

    static void rectangular(Sample* buffer, int bufferSize)
    {
        for (int i = 0; i < bufferSize; i++)
        {
            buffer[i] = static_cast<Sample>(1);
        }
    }

    //Matlab 'periodic': the max is at index bufferSize/2
    static void triangular(Sample* buffer, int bufferSize)
    {
        bool isEven = (bufferSize % 2) == 0;

        if (isEven)
        {
            double bufferSizeHalf = bufferSize / 2;

            for (int i = 0; i < bufferSize; i++)
            {
                buffer[i] = static_cast<Sample>(1.0 - std::abs(static_cast<double>(i) - bufferSizeHalf) / bufferSizeHalf);
            }
        }
    }

    //Matlab hanning(,'periodic'): the max is at index bufferSize/2
    static void hann(Sample* buffer, int bufferSize)
    {
        bool isEven = (bufferSize % 2) == 0;

        if (isEven)
        {
            for (int i = 0; i < bufferSize; i++)
            {
                buffer[i] = static_cast<Sample>(0.5 * (1.0 - std::cos(2.0 * M_PI * static_cast<double>(i) / static_cast<double>(bufferSize))));
            }
        }
    }

    static void hannSquareRoot(Sample* buffer, int bufferSize)
    {
        hann(buffer, bufferSize);

        for (int i = 0; i < bufferSize; i++)
        {
            buffer[i] = static_cast<Sample>(std::sqrt(buffer[i]));
        }
    }

    static void circularShift(Sample* buffer, int bufferSize)
    {
        bool isEven = (bufferSize % 2) == 0;

        if (isEven)
        {
            Sample sample;
            int bufferSizeHalf = bufferSize / 2;

            for (int i = 0; i < bufferSizeHalf; i++)
            {
                sample = buffer[i + bufferSizeHalf];

                buffer[i + bufferSizeHalf] = buffer[i];
                buffer[i] = sample;
            }
        }
    }

    /* Calculate the coefficient to apply on synthesized samples based on the type of window and the overlapping ratio*/
    static Sample calculateOverlappingNormalizationCoefficient(Type windowType, float overlappingRatio)
    {
        Sample coefficient = static_cast<Sample>(1.0);

        switch (windowType)
        {
        case Rectangular:
            coefficient = static_cast<Sample>(1.0) / static_cast<Sample>(overlappingRatio);
            break;

        case Triangle:
            coefficient = static_cast<Sample>(2.0) / static_cast<Sample>(overlappingRatio);
            break;

        case Hann:
            coefficient = static_cast<Sample>(2.0) / static_cast<Sample>(overlappingRatio);
            break;

        case HannSquareRoot:
            coefficient = std::sqrt(static_cast<Sample>(2.0) / static_cast<Sample>(overlappingRatio));
            break;
        
        case numberOfWindows:
            // DO NOTHING
            break;
        }

        return coefficient;
    }

};

#endif
