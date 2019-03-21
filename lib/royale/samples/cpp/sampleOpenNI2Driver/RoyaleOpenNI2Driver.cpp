/****************************************************************************\
* Copyright (C) 2017 Infineon Technologies
*
* THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
*
\****************************************************************************/

#if defined(__STDC_LIB_EXT1__) || defined(_MSC_VER)
#define HAVE_STRCPY_S
#define __STDC_WANT_LIB_EXT1__ 1
#endif

#include "RoyaleOpenNI2Driver.hpp"
#include "RoyaleOpenNI2Device.hpp"

#include <Driver/OniDriverAPI.h>

#include <algorithm>
#include <thread>
#include <chrono>

#ifdef HAVE_STRCPY_S
#include <string.h>
#else
#include <cstring>
#endif


#define XN_NEW(type, ...)               new type(__VA_ARGS__)
#define XN_DELETE(p)                    delete (p)

ONI_EXPORT_DRIVER (royale::openni2::RoyaleOpenNI2Driver);

using namespace royale::openni2;

RoyaleOpenNI2Driver::RoyaleOpenNI2Driver (OniDriverServices *pDriverServices)
    : DriverBase (pDriverServices)
{
}

oni::driver::DeviceBase *RoyaleOpenNI2Driver::deviceOpen (const char *uri, const char * /*mode*/)
{
    // get currently connected cameras... again.
    CameraManager manager;
    royale::Vector<royale::String> camlist (manager.getConnectedCameraList());

    // Now check if one matches uri.
    royale::String uriAsString (uri);
    for (auto cameraName : camlist)
    {
        if (cameraName == uriAsString)
        {
            auto royaleDevice = manager.createCamera (cameraName);
            auto dev = new RoyaleOpenNI2Device (std::move (royaleDevice), getServices());
            return dev;
        }
    }

    getServices().errorLoggerAppend ("Looking for '%s'", uri);
    return NULL;
}

void RoyaleOpenNI2Driver::deviceClose (oni::driver::DeviceBase *pDevice)
{
    delete pDevice;
}

void RoyaleOpenNI2Driver::shutdown()
{
}

OniStatus RoyaleOpenNI2Driver::initialize (oni::driver::DeviceConnectedCallback connectedCallback,
        oni::driver::DeviceDisconnectedCallback disconnectedCallback,
        oni::driver::DeviceStateChangedCallback deviceStateChangedCallback,
        void *pCookie)
{
    // call super...
    DriverBase::initialize (connectedCallback, disconnectedCallback, deviceStateChangedCallback, pCookie);

    // get currently connected cameras...
    CameraManager manager;
    auto camlist = manager.getConnectedCameraList();

    // ...and report them to OpenNI2.
    OniDeviceInfo deviceInfo;
    for (auto cameraName : camlist)
    {
#ifdef HAVE_STRCPY_S
        strcpy_s (deviceInfo.uri, ONI_MAX_STR, cameraName.c_str());
        strcpy_s (deviceInfo.vendor, ONI_MAX_STR, "Infineon");
        strcpy_s (deviceInfo.name, ONI_MAX_STR, "ToF Sensor");
#else
        strncpy (deviceInfo.uri, cameraName.c_str(), ONI_MAX_STR);
        strncpy (deviceInfo.vendor, "Infineon", ONI_MAX_STR);
        strncpy (deviceInfo.name, "ToF Sensor", ONI_MAX_STR);
#endif

        deviceInfo.usbVendorId = 0;
        deviceInfo.usbProductId = 0;

        deviceConnected (&deviceInfo);
        deviceStateChanged (&deviceInfo, 0);
    }
    return ONI_STATUS_OK;
}
