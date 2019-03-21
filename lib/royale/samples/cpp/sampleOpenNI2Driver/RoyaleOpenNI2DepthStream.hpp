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
        class RoyaleOpenNI2Device;

        class RoyaleOpenNI2DepthStream : public oni::driver::StreamBase,
            private royale::IDepthDataListener
        {
        public:
            explicit RoyaleOpenNI2DepthStream (RoyaleOpenNI2Device &device);
            ~RoyaleOpenNI2DepthStream() override;

            OniStatus start() override;
            void stop() override;
            OniStatus getProperty (int propertyId, void *data, int *pDataSize) override;
            OniStatus setProperty (int propertyId, const void *data, int dataSize) override;

        private:
            void onNewData (const royale::DepthData *data) override;

        protected:
            RoyaleOpenNI2Device      &m_device;
            int                       m_index;
            std::chrono::microseconds m_lastTime;

        };

    } // namespace openni2
} // namespace royale
