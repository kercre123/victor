/****************************************************************************\
* Copyright (C) 2016 Infineon Technologies
*
* THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
\****************************************************************************/

#pragma once

#include <collector/IFrameCaptureListener.hpp>
#include <royale/ProcessingFlag.hpp>
#include <royale/StreamId.hpp>

namespace royale
{
    class IRecord : public royale::collector::IFrameCaptureListener
    {
    public :
        virtual ~IRecord() = default;

        /**
         * Indicates that recording is currently active
         */
        virtual bool isRecording() = 0;

        /**
        *  Set/alter processing parameters in order to control the data output when the recording is replayed.
        *  A list of processing flags is available as an enumeration. The `Variant` data type can take float, int, or bool.
        *  Please make sure to set the proper `Variant` type for the enum.
        */
        virtual void setProcessingParameters (const royale::ProcessingParameterVector &parameters, const royale::StreamId streamId) = 0;

        /**
        *  Resets the internal processing parameters of the recording. Otherwise the recording would not know when it is safe
        *  to discard old parameter sets.
        */
        virtual void resetParameters() = 0;

        /**
        *  Starts a recording under the given filename. If there is already a recording running
        *  it will be stopped and the new recording will start.
        *  @param filename Filename which should be used
        *  @param calibrationData Calibration data used for the recording
        *  @param imagerSerial Serial number of the imager used for the recording
        *  @param numFrames Number of frames which should be recorded (0 equals infinite frames)
        *  @param frameSkip Number of frames which should be skipped after every recorded frame (0 equals all frames will be recorded)
        *  @param msSkip Time which should be skipped after every recorded frame (0 equals all frames will be recorded)
        */
        virtual void startRecord (const royale::String &filename, const std::vector<uint8_t> &calibrationData,
                                  const royale::String &imagerSerial,
                                  const uint32_t numFrames = 0, const uint32_t frameSkip = 0, const uint32_t msSkip = 0) = 0;
        /**
        *  Stops the current recording.
        *  If no recording is running the function will return.
        */
        virtual void stopRecord() = 0;

        /**
         * Set/alter the current IFrameCaptureListener. This can only be done if no recording is happening.
         * If the system is recording, the listener will not be changed and false is returned.
         */
        virtual bool setFrameCaptureListener (royale::collector::IFrameCaptureListener *captureListener) = 0;
    };
}
