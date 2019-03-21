/****************************************************************************\
 * Copyright (C) 2017 Infineon Technologies & pmdtechnologies ag
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#include <royale.hpp>
#include <sample_utils/EventReporter.hpp>
#include <sample_utils/PlatformResources.hpp>

int main()
{
    using namespace sample_utils;
    using namespace std;

    // Windows requires that the application allocate these, not the DLL.
    PlatformResources resources;

    // this represents the main camera device object
    std::unique_ptr<royale::ICameraDevice> cameraDevice;

    // the camera manager will query for a connected camera
    {
        royale::CameraManager manager;
        sample_utils::EventReporter eventReporter;

        manager.registerEventListener (&eventReporter);

        royale::Vector<royale::String> camlist (manager.getConnectedCameraList());
        cout << "Detected " << camlist.size() << " camera(s)." << endl;
        bool camlistEmpty = camlist.empty();

        if (!camlistEmpty)
        {
            cameraDevice = manager.createCamera (camlist[0]);
        }

        camlist.clear();
        // EventReporter will be deallocated before manager. So eventReporter must be
        // unregistered. Declare eventReporter before manager to make the next call unnecessary.
        manager.unregisterEventListener ();

        if (camlistEmpty)
        {
            cerr  << "No suitable camera device detected." << endl
                  << "Please make sure that a supported camera is plugged in, all drivers are "
                  << "installed, and you have proper USB permission" << endl;
            return 1;
        }
    }
    // the camera device is now available and CameraManager can be deallocated here

    if (cameraDevice == nullptr)
    {
        cerr << "Cannot create the camera device" << endl;
        return 1;
    }

    // IMPORTANT: call the initialize method before working with the camera device
    auto status = cameraDevice->initialize();
    if (status != royale::CameraStatus::SUCCESS)
    {
        cerr << "Cannot initialize the camera device, error string : " << getErrorString (status) << endl;
        return 1;
    }

    royale::String id;
    royale::String name;
    uint16_t maxSensorWidth;
    uint16_t maxSensorHeight;

    status = cameraDevice->getId (id);
    if (royale::CameraStatus::SUCCESS != status)
    {
        cerr << "failed to get ID: " << getErrorString (status) << endl;
        return 1;
    }

    status = cameraDevice->getCameraName (name);
    if (royale::CameraStatus::SUCCESS != status)
    {
        cerr << "failed to get name: " << getErrorString (status) << endl;
        return 1;
    }

    status = cameraDevice->getMaxSensorWidth (maxSensorWidth);
    if (royale::CameraStatus::SUCCESS != status)
    {
        cerr << "failed to get max sensor width: " << getErrorString (status) << endl;
        return 1;
    }

    status = cameraDevice->getMaxSensorHeight (maxSensorHeight);
    if (royale::CameraStatus::SUCCESS != status)
    {
        cerr << "failed to get max sensor height: " << getErrorString (status) << endl;
        return 1;
    }

    royale::Vector<royale::String> useCases;
    status = cameraDevice->getUseCases (useCases);
    if (royale::CameraStatus::SUCCESS != status)
    {
        cerr << "failed to get available use cases: " << getErrorString (status) << endl;
        return 1;
    }

    royale::Vector<royale::Pair<royale::String, royale::String>> cameraInfo;
    status = cameraDevice->getCameraInfo (cameraInfo);
    if (royale::CameraStatus::SUCCESS != status)
    {
        cerr << "failed to get camera info: " << getErrorString (status) << endl;
        return 1;
    }

    // display some information about the connected camera
    cout << "====================================" << endl;
    cout << "        Camera information"           << endl;
    cout << "====================================" << endl;
    cout << "Id:              " << id << endl;
    cout << "Type:            " << name << endl;
    cout << "Width:           " << maxSensorWidth << endl;
    cout << "Height:          " << maxSensorHeight << endl;
    cout << "Operation modes: " << useCases.size() << endl;

    const auto listIndent = std::string ("    ");
    const auto noteIndent = std::string ("        ");

    for (size_t i = 0; i < useCases.size(); ++i)
    {
        cout << listIndent << useCases[i] << endl;

        uint32_t streamCount = 0;
        status = cameraDevice->getNumberOfStreams (useCases[i], streamCount);
        if (royale::CameraStatus::SUCCESS == status && streamCount > 1)
        {
            cout << noteIndent << "this operation mode has " << streamCount << " streams" << endl;
        }
    }

    cout << "CameraInfo items: " << cameraInfo.size() << endl;
    for (size_t i = 0; i < cameraInfo.size(); ++i)
    {
        cout << listIndent << cameraInfo[i] << endl;
    }

    return 0;
}
