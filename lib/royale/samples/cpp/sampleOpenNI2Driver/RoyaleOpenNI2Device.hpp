/****************************************************************************\
* Copyright (C) 2017 Infineon Technologies
*
* THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
\****************************************************************************/

#pragma once

#include <royale.hpp>
#include <OpenNI.h>
#include <Driver/OniDriverAPI.h>

#include "RoyaleOpenNI2DepthStream.hpp"

#include <map>

namespace royale
{
    namespace openni2
    {
        class RoyaleOpenNI2Device : public oni::driver::DeviceBase
        {
        public:
            RoyaleOpenNI2Device (std::unique_ptr<royale::ICameraDevice> camera, oni::driver::DriverServices &driverServices);
            OniStatus getSensorInfoList (OniSensorInfo **pSensors, int *numSensors) override;
            oni::driver::StreamBase *createStream (OniSensorType sensorType) override;
            void destroyStream (oni::driver::StreamBase *pStream) override;
            OniStatus  getProperty (int propertyId, void *data, int *pDataSize) override;

            // for use by the stream
            royale::ICameraDevice &getCamera();

            // Called by the stream
            OniStatus GetVideoMode (OniVideoMode *pVideoMode);
            OniStatus SetVideoMode (OniVideoMode *pVideoMode);

            // noncopyable
            RoyaleOpenNI2Device (const RoyaleOpenNI2Device &) = delete;
            void operator= (const RoyaleOpenNI2Device &) = delete;
        private:
            std::unique_ptr<royale::ICameraDevice> m_camera;
            int m_numSensors;
            OniSensorInfo m_sensors[1];
            oni::driver::DriverServices &m_driverServices;
            RoyaleOpenNI2DepthStream m_stream;

            std::map<std::string, OniVideoMode> m_videoModes; // usecase to video mode mapping
        };

    } // namespace openni2
} // namespace royale
