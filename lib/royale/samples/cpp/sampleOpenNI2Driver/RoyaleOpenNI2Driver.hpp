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

namespace royale
{
    namespace openni2
    {
        class RoyaleOpenNI2Driver : public oni::driver::DriverBase
        {
        public:
            explicit RoyaleOpenNI2Driver (OniDriverServices *pDriverServices);
            virtual oni::driver::DeviceBase *deviceOpen (const char *uri, const char * /*mode*/) override;
            virtual void deviceClose (oni::driver::DeviceBase *pDevice) override;
            void shutdown() override;
            OniStatus initialize (oni::driver::DeviceConnectedCallback connectedCallback,
                                  oni::driver::DeviceDisconnectedCallback disconnectedCallback,
                                  oni::driver::DeviceStateChangedCallback deviceStateChangedCallback,
                                  void *pCookie) override;
        };

    } // namespace openni2
} // namespace royale
