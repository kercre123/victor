#!/usr/bin/python3

# Copyright (C) 2018 Infineon Technologies & pmdtechnologies ag
#
# THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
# KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
# PARTICULAR PURPOSE.

import os
if os.name == 'nt':
    import pythoncom

class PlatformHelper:
    """A wrapper for the CoInitialize/CoUninitialize calls needed by Windows to operate
    UVC cameras
    """

    def __init__ (self):
        if os.name == 'nt':
            pythoncom.CoInitialize()
    def __del__(self):                
        if os.name == 'nt':
            pythoncom.CoUninitialize()

if __name__ == "__main__":
    print ("roypy_platform_utils is a utility module, it doesn't expect to be run directly")
