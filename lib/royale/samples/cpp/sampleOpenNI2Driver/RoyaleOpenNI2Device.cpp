/****************************************************************************\
* Copyright (C) 2017 Infineon Technologies
*
* THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
\****************************************************************************/

#include "RoyaleOpenNI2Device.hpp"

#include <Driver/OniDriverAPI.h>

#include <algorithm>
#include <thread>
#include <chrono>

using namespace royale::openni2;

RoyaleOpenNI2Device::RoyaleOpenNI2Device (std::unique_ptr<royale::ICameraDevice> camera, oni::driver::DriverServices &driverServices)
    : m_camera (std::move (camera)),
      m_driverServices (driverServices),
      m_stream (*this)
{
    royale::Vector<royale::String> useCases;
    if (m_camera->getUseCases (useCases) != royale::CameraStatus::SUCCESS)
    {
        throw std::runtime_error ("failed to list usecases");
    }

    royale::String defaultUseCase;
    m_camera->getCurrentUseCase (defaultUseCase);

    m_numSensors = 1;

    auto n_usecases = useCases.count();

    m_sensors[0].sensorType = ONI_SENSOR_DEPTH;
    m_sensors[0].numSupportedVideoModes = static_cast<int> (n_usecases);
    m_sensors[0].pSupportedVideoModes = new OniVideoMode[n_usecases];


    for (size_t idx = 0; idx < n_usecases; ++idx)
    {
        auto &vidmode = m_sensors[0].pSupportedVideoModes[idx];
        const auto &usecase = useCases.at (idx);

        m_camera->setUseCase (usecase);

        vidmode.pixelFormat = ONI_PIXEL_FORMAT_DEPTH_1_MM;

        uint16_t resx;
        if (m_camera->getMaxSensorWidth (resx) != royale::CameraStatus::SUCCESS)
        {
            throw std::runtime_error ("failed getting x res");
        }
        vidmode.resolutionX = resx;
        uint16_t resy;
        if (m_camera->getMaxSensorHeight (resy) != royale::CameraStatus::SUCCESS)
        {
            throw std::runtime_error ("failed getting y res");
        }
        vidmode.resolutionY = resy;
        uint16_t fps;
        if (m_camera->getFrameRate (fps) != royale::CameraStatus::SUCCESS)
        {
            throw std::runtime_error ("failed getting fps");
        }
        vidmode.fps = fps;

        m_videoModes[usecase.toStdString()] = vidmode;
    }

    m_camera->setUseCase (defaultUseCase);

}

OniStatus RoyaleOpenNI2Device::getSensorInfoList (OniSensorInfo **pSensors, int *numSensors)
{
    *numSensors = m_numSensors;
    *pSensors = m_sensors;

    return ONI_STATUS_OK;
}

oni::driver::StreamBase *RoyaleOpenNI2Device::createStream (OniSensorType sensorType)
{
    if (sensorType == ONI_SENSOR_DEPTH)
    {
        return &m_stream;
    }
    m_driverServices.errorLoggerAppend ("RoyaleOpenNI2Device: Can't create a stream of type %d", sensorType);
    return NULL;
}

void RoyaleOpenNI2Device::destroyStream (oni::driver::StreamBase *pStream)
{
    // no-op, our (singleton) stream is part of the device.
}

OniStatus  RoyaleOpenNI2Device::getProperty (int propertyId, void *data, int *pDataSize)
{
    OniStatus rc = ONI_STATUS_OK;

    switch (propertyId)
    {
        case ONI_DEVICE_PROPERTY_DRIVER_VERSION:
            {
                if (*pDataSize == sizeof (OniVersion))
                {
                    OniVersion *version = (OniVersion *) data;

                    unsigned major, minor, patch, build;
                    royale::getVersion (major, minor, patch, build);

                    version->major = static_cast<int> (major);
                    version->minor = static_cast<int> (minor);
                    version->maintenance = static_cast<int> (patch);
                    version->build = static_cast<int> (build);
                }
                else
                {
                    m_driverServices.errorLoggerAppend ("Unexpected size: %d != %d\n", *pDataSize, sizeof (OniVersion));
                    rc = ONI_STATUS_ERROR;
                }
            }
            break;
        default:
            m_driverServices.errorLoggerAppend ("Unknown property: %d\n", propertyId);
            rc = ONI_STATUS_ERROR;
    }
    return rc;
}

// for use by the stream
royale::ICameraDevice &RoyaleOpenNI2Device::getCamera()
{
    return *m_camera;
}


OniStatus RoyaleOpenNI2Device::GetVideoMode (OniVideoMode *pVideoMode)
{
    royale::String usecase;
    if (m_camera->getCurrentUseCase (usecase) != royale::CameraStatus::SUCCESS)
    {
        return ONI_STATUS_ERROR;
    }

    auto uc_it = m_videoModes.find (usecase.toStdString());
    if (uc_it == m_videoModes.end())
    {
        // ...weird.
        return ONI_STATUS_ERROR;
    }

    *pVideoMode = uc_it->second;

    return ONI_STATUS_OK;
}

OniStatus RoyaleOpenNI2Device::SetVideoMode (OniVideoMode *pVideoMode)
{
    // Find a matching usecase for the video mode.
    for (const auto &vm : m_videoModes)
    {
        const auto &videoMode = vm.second;

        if (videoMode.fps != pVideoMode->fps)
        {
            continue;
        }
        if (videoMode.pixelFormat != pVideoMode->pixelFormat)
        {
            continue;
        }
        if (videoMode.resolutionX != pVideoMode->resolutionX)
        {
            continue;
        }
        if (videoMode.resolutionY != pVideoMode->resolutionY)
        {
            continue;
        }

        if (m_camera->setUseCase (vm.first) != royale::CameraStatus::SUCCESS)
        {
            return ONI_STATUS_ERROR;
        }
        return ONI_STATUS_OK;
    }

    // Didn't find anything matching.
    return ONI_STATUS_BAD_PARAMETER;
}

