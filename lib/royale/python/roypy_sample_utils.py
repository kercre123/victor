#!/usr/bin/python3

# Copyright (C) 2018 Infineon Technologies & pmdtechnologies ag
#
# THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
# KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
# PARTICULAR PURPOSE.

"""The Roypy library itself is a wrapper for the C++ API, and so its API is stable.  This file
contains utilities which are useful to multiple samples, and could be used as snippets in other
applications"""

import argparse
import roypy

def add_camera_opener_options (parser):
    if not isinstance (parser, argparse.ArgumentParser):
        print ("This method only supports argparse.ArgumentParser")
    parser.add_argument ("--code", default=None, help="access code")
    parser.add_argument ("--rrf", default=None, help="play a recording instead of opening the camera")
    parser.add_argument ("--cal", default=None, help="load an alternate calibration file (requires level 2 access)")

class CameraOpener:
    """A wrapper around roypy which adds support for the samples' common command line arguments
    (--code, --rrf, etc).

    The caller is expected to create its own argparse.ArgumentParser, and then call
    add_camera_opener_options to add the CameraOpener-supported options to it before calling
    ArgumentParser.parse_args().  Pass the parsed options to CameraOpener's constructor.

    Some samples require a minimum access level, for example to set processing options or to receive
    raw data.  If a minimum access level is required, and no --code option is supplied on the
    command line, then this utility class will look for a code lookup table in activation_code.py.
    """

    def __init__ (self, options, min_access_level=None):
        """Create an instance of CameraOpener, the required options argument should be the result of
        ArgumentParser.parse_args, or a similar option container.
        """
        self._options = options
        self._min_access_level = min_access_level

    def _get_camera_manager (self):
        """Creates an instance of CameraManager, ensuring that it's at least the minimum access
        level required. The activation codes can be provided via the command-line --code argument,
        or by putting them in the table in activation_code.py.

        If a --code argument is given on the command line, it will always be used; this can be used
        to run a sample at a higher access level than its minimum. 
        """
        # The next line tests for None, to allow '--code ""' on the command line to force access level 1
        if self._options.code is not None:
            code = self._options.code
            if (self._min_access_level is not None) and (roypy.CameraManager.getAccessLevel(code) < self._min_access_level):
                raise RuntimeError ("This example requires at least access level %d, and the --code argument is less than that"
                        % (self._min_access_level))
        elif self._min_access_level is not None:
            try:
                from activation_code import activation_code_table
                code = activation_code_table[self._min_access_level]
                if roypy.CameraManager.getAccessLevel(code) != self._min_access_level:
                    raise RuntimeError ("activation_code_table contains the incorrect access code for level %d"
                            % (self._min_access_level))
            except KeyError:
                raise RuntimeError ("activation_code_table does not contain an access code for level %d"
                            % (self._min_access_level))
            except ImportError:
                raise RuntimeError ("This example requires at least access level %d, either via a --code argument or by importing it from activation_code.py"
                        % (self._min_access_level))
        else:
            code = ""

        return roypy.CameraManager (code)

    def _pre_initialize (self, cam):
        """Loading an alternate calibration is done after createCamera() but before initialize().
        This method handles all changes at that time.
        """
        if (self._options.cal):
            cam.setCalibrationData (self._options.cal)

    def open_hardware_camera (self):
        """Search for a connected camera and open it, handling the access level.
        """
        c = self._get_camera_manager()
        l = c.getConnectedCameraList()

        print("Number of cameras connected: ", l.size())
        if l.size() == 0:
            raise RuntimeError ("No cameras connected")

        cam = c.createCamera(l[0])
        self._pre_initialize (cam)
        cam.initialize()

        return cam

    def open_recording (self, rrf):
        """Open the recording whose filename is passed in the rrf argument, with handling for the
        access level options.
        """
        print("Filename: " + rrf)
        cam = self._get_camera_manager().createCamera (rrf)
        self._pre_initialize (cam)
        cam.initialize()
        return cam

    def open_camera (self):
        """If the command line options included an --rrf option, open that recording, otherwise
        search for a connected camera and open it.

        If the sample has specified a minimum required access level, CameraOpener will raise an
        error if it can't enable this access level in Royale.  The activation codes can be provided
        via the command-line --code argument, or by putting them in the table in activation_code.py.

        If a --code argument is given on the command line, it will always be used; this can be used
        to run a sample at a higher access level than its minimum. 
        """
        if self._options.rrf:
            cam = self.open_recording (self._options.rrf)
        else:
            cam = self.open_hardware_camera ()
        return cam

if __name__ == "__main__":
    print ("roypy_sample_utils is a utility module, it doesn't expect to be run directly")
