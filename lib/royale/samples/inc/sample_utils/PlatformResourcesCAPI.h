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

#include <stdbool.h>
#include <stdio.h>

#include <StatusCAPI.h>

#ifdef _WINDOWS
#include <windows.h>
#endif


/**
 * Some platforms (and certain frameworks on those platforms) require the application to call an
 * initialization method before the library can use certain features.
 *
 * The only one currently affecting us is Windows COM, which is needed for the UVC camera
 * support on Windows.
 *
 * Qt will also create these resources, in a Qt app the application does not need to create them
 * (and Qt will fail to start if the application creates them with conflicting settings).
 */
royale_camera_status royale_platform_resources_initialize (void)
{
#ifdef _WINDOWS
    HRESULT hr = CoInitializeEx (NULL, COINIT_APARTMENTTHREADED);
    if (FAILED (hr))
    {
        return ROYALE_STATUS_RESOURCE_ERROR;
    }
#endif
    return ROYALE_STATUS_SUCCESS;
}

/**
 * If platform_resources_initialize returned ROYALE_STATUS_SUCCESS then this should be called
 * before exit.  If _initialize returned an error, this should not be called.
 */
void royale_platform_resources_uninitialize (void)
{
#ifdef _WINDOWS
    CoUninitialize ();
#endif
}
