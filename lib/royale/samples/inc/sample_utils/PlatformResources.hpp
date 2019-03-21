/****************************************************************************\
 * Copyright (C) 2017 Infineon Technologies & pmdtechnologies ag
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#pragma once

#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace sample_utils
{
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
    class PlatformResources
    {
#ifdef _WIN32
    public:
        PlatformResources () :
            m_initializedSuccessfully {false}
        {
            auto hr = CoInitializeEx (NULL, COINIT_APARTMENTTHREADED);
            if (FAILED (hr))
            {
                std::cout << "Can not initialize for the COM framework, UVC devices will not work" << std::endl;
            }
            else
            {
                m_initializedSuccessfully = true;
            }
        }

        ~PlatformResources ()
        {
            if (m_initializedSuccessfully)
            {
                CoUninitialize ();
            }
        }

    private:
        bool m_initializedSuccessfully;
#else
    public:
        PlatformResources () = default;
        ~PlatformResources ()
        {
            // non-trivial destructor to avoid an "unused variable platformResources" warning
            (void) this;
        }
#endif
    };
}
