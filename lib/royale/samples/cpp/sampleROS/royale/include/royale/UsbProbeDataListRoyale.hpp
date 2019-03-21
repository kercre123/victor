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

#include <usb/config/UsbProbeData.hpp>
#include <factory/IProcessingParameterMapFactory.hpp>

#include <memory>

namespace royale
{
    namespace config
    {
        /**
         * Returns the list of identifiers and module configurations for every USB camera that's
         * supported by default by Royale.
         */
        ROYALE_API royale::usb::config::UsbProbeDataList getUsbProbeDataRoyale();

        /**
         * Returns a factory which can provide the processing parameters for every camera that is
         * included in getUsbProbeDataRoyale and which links external processing parameters by ID.
         */
        ROYALE_API std::shared_ptr<royale::factory::IProcessingParameterMapFactory> getProcessingParameterMapFactoryRoyale();
    }
}
