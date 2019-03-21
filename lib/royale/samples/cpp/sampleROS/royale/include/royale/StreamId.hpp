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

#include <cstdint>

namespace royale
{
    /**
     * The StreamId uniquely identifies a stream of measurements within a usecase.
     *
     * A stream is a sequence of periodic measurements having the same depth range, exposure times
     * and other settings. Most usecases will only produce a single stream, but with mixed-mode
     * usecases, there may be more than one. A typical mixed-mode usecase may for example include
     * one stream designed for hand tracking (which needs a high frame rate but can work with
     * reduced depth range) and another one for environment scanning (full depth range at a lower
     * frame rate).
     *
     * StreamId 0 is not a valid StreamId, but can (for backward compatibility) be used as
     * referring to the single stream contained in non mixed mode usecases in most API functions
     * that expect a StreamId. This allows applications that don't make use of mixed mode to
     * work without having to deal with StreamIds. Applications that need to use mixed mode
     * usecases will have to provide the correct IDs in the API as 0 is not accepted as StreamId
     * if a mixed mode usecase is active.
     */
    using StreamId = uint16_t;
}
