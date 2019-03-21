/****************************************************************************\
 * Copyright (C) 2018 Infineon Technologies & pmdtechnologies ag
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#pragma once

#include <royale/IEvent.hpp>

namespace sample_utils
{
    /**
     * This class reports on events from Royale. Depending on the event's severity, this class
     * writes the description of the event to standard output or standard error. Information and
     * warnings are written to standard output. Non-fatal errors and fatal errors are written to
     * standard error.
     */
    class EventReporter : public royale::IEventListener
    {
    public:
        virtual ~EventReporter() = default;

        /**
         * @inheritdoc
         */
        virtual void onEvent (std::unique_ptr<royale::IEvent> &&event) override
        {
            royale::EventSeverity severity = event->severity();

            switch (severity)
            {
                case royale::EventSeverity::ROYALE_INFO:
                case royale::EventSeverity::ROYALE_WARNING:
                    std::cout << event->describe() << std::endl;
                    break;
                case royale::EventSeverity::ROYALE_ERROR:
                case royale::EventSeverity::ROYALE_FATAL:
                    std::cerr << event->describe() << std::endl;
                    break;
            }
        }
    };
}
