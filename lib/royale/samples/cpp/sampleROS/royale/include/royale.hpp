/****************************************************************************\
 * Copyright (C) 2015 Infineon Technologies
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#pragma once

// This is the top level royale include file

#include <royale/CameraManager.hpp>
#include <royale/ICameraDevice.hpp>
#include <royale/Status.hpp>
#include <royale/String.hpp>

namespace royale
{
    /**
     * Get the library version of Royale.
     *
     * \param major Contains major version number on return
     * \param minor Contains minor version number on return
     * \param patch Contains patch number on return
     */
    ROYALE_API void getVersion (unsigned &major, unsigned &minor, unsigned &patch);
    /**
     * Get library version and build number of Royale.
     *
     * \param major Contains major version number on return
     * \param minor Contains minor version number on return
     * \param patch Contains patch number on return
     * \param build Contains build number on return
     */
    ROYALE_API void getVersion (unsigned &major, unsigned &minor, unsigned &patch, unsigned &build);
    /**
     * Get library version, build number and SCM revision of Royale.
     *
     * \param major Contains major version number on return
     * \param minor Contains minor version number on return
     * \param patch Contains patch number on return
     * \param build Contains build number on return
     * \param scmRevision Contains SCM revision on return
     * The SCM revision consists of a git hash (with "-dirty" flag appended for uncommitted changes);
     * it may be empty for non-SCM builds.
     */
    ROYALE_API void getVersion (unsigned &major, unsigned &minor, unsigned &patch, unsigned &build, royale::String &scmRevision);

    /**
     * Get a human-readable description for a given error message.
     *
     * Note: These descriptions are in English and are intended to help developers,
     * they're not translated to the current locale.
     */
    ROYALE_API royale::String getErrorString (royale::CameraStatus status);
}
