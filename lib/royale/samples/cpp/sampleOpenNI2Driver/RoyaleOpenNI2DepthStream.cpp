/****************************************************************************\
* Copyright (C) 2017 Infineon Technologies
*
* THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
\****************************************************************************/

#include "RoyaleOpenNI2DepthStream.hpp"
#include "RoyaleOpenNI2Device.hpp"

#include <Driver/OniDriverAPI.h>

#include <algorithm>
#include <thread>
#include <chrono>

using namespace royale::openni2;

RoyaleOpenNI2DepthStream::RoyaleOpenNI2DepthStream (RoyaleOpenNI2Device &device)
    : m_device (device),
      m_index (0),
      m_lastTime()
{
    auto status = m_device.getCamera().initialize();
    if (status != CameraStatus::SUCCESS)
    {
        std::cerr << "Cannot initialize the camera device, error string : " << getErrorString (status) << std::endl;
    }

    m_device.getCamera().registerDataListener (this);
}

RoyaleOpenNI2DepthStream::~RoyaleOpenNI2DepthStream()
{
    stop();
    m_device.getCamera().unregisterDataListener();
}

OniStatus RoyaleOpenNI2DepthStream::start()
{
    auto ret = m_device.getCamera().startCapture();

    if (ret == royale::CameraStatus::SUCCESS)
    {
        return ONI_STATUS_OK;
    }
    else
    {
        std::cout << "...fail to start capture, ret=" << getErrorString (ret) << std::endl;

        return ONI_STATUS_ERROR;
    }
}

void RoyaleOpenNI2DepthStream::stop()
{
    m_device.getCamera().stopCapture();
}

OniStatus RoyaleOpenNI2DepthStream::getProperty (int propertyId, void *data, int *pDataSize)
{
    if (propertyId == ONI_STREAM_PROPERTY_VIDEO_MODE)
    {
        if (*pDataSize != sizeof (OniVideoMode))
        {
            printf ("Unexpected size: %d != %d\n", *pDataSize, (int) sizeof (OniVideoMode));
            return ONI_STATUS_ERROR;
        }
        return m_device.GetVideoMode ( (OniVideoMode *) data);
    }
    return ONI_STATUS_NOT_IMPLEMENTED;
}

OniStatus RoyaleOpenNI2DepthStream::setProperty (int propertyId, const void *data, int dataSize)
{
    if (propertyId == ONI_STREAM_PROPERTY_VIDEO_MODE)
    {
        if (dataSize != sizeof (OniVideoMode))
        {
            printf ("Unexpected size: %d != %d\n", dataSize, (int) sizeof (OniVideoMode));
            return ONI_STATUS_ERROR;
        }
        return m_device.SetVideoMode ( (OniVideoMode *) data);
    }
    return ONI_STATUS_NOT_IMPLEMENTED;
}

void RoyaleOpenNI2DepthStream::onNewData (const royale::DepthData *data)
{

    auto frame = getServices().acquireFrame();

    if (!frame)
    {
        // didn't get a free buffer...
        return;
    }

    // Clear frame
    memset (frame->data, 0, static_cast<size_t> (frame->dataSize));

    size_t n_pixels = data->width * data->height;
    // Sanity check...
    if (static_cast<size_t> (frame->dataSize) < n_pixels * sizeof (openni::DepthPixel))
    {
        // Buffer too small...
        getServices().releaseFrame (frame);
        return;
    }

    // copy/convert
    for (size_t idx = 0; idx < n_pixels; ++idx)
    {
        static_cast<openni::DepthPixel *> (frame->data) [idx] = static_cast<openni::DepthPixel> (1000.0 * data->points[idx].z);
    }

    // Fill metadata
    frame->frameIndex = ++m_index;
    frame->videoMode.pixelFormat = ONI_PIXEL_FORMAT_DEPTH_1_MM;
    frame->videoMode.resolutionX = data->width;
    frame->videoMode.resolutionY = data->height;
    frame->videoMode.fps = 30;
    frame->stride = static_cast<int> (data->width * sizeof (OniDepthPixel));
    frame->height = data->height;
    frame->width = data->width;
    frame->sensorType = OniSensorType::ONI_SENSOR_DEPTH;
    frame->timestamp = static_cast<uint64_t> (data->timeStamp.count());
    frame->cropOriginX = 0;
    frame->cropOriginY = 0;
    frame->croppingEnabled = FALSE;

    // calculate the current fps
    frame->videoMode.fps = (int) (1000000 / (std::chrono::duration_cast<std::chrono::microseconds> (data->timeStamp - m_lastTime)).count());
    m_lastTime = data->timeStamp;


    // ...and hand over to openNI
    raiseNewFrame (frame);
    getServices().releaseFrame (frame);
}
