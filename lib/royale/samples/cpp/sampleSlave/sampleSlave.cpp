/****************************************************************************\
 * Copyright (C) 2018 Infineon Technologies & pmdtechnologies ag
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#include <royale.hpp>
#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

#include <sample_utils/PlatformResources.hpp>

using namespace sample_utils;
using namespace std;

namespace
{
    class SlaveListener : public royale::IDepthDataListener
    {
        void onNewData (const royale::DepthData *data) override
        {
            cout << "Received data for the slave ..." << endl;

            // Do something with the data
        }
    };
}

/**
 * This example shows how to handle slave connection. It tries to open one cameras and
 * define it as a slave.
 */
int main (int argc, char **argv)
{
    // Windows requires that the application allocate these, not the DLL.
    PlatformResources resources;

    // the data listener which will receive callbacks.
    SlaveListener slaveListener;

    // represent the main camera device objects
    unique_ptr<royale::ICameraDevice> slaveCameraDevice;

    // the camera manager will query for a connected camera
    {
        royale::CameraManager manager;

        auto camlist = manager.getConnectedCameraList();
        cout << "Detected " << camlist.size() << " camera(s)." << endl;

        if (camlist.size() < 1)
        {
            cerr << "No device has been found! Please check the connection or the camera device!" << endl;
            return 1;
        }

        royale::String slaveId = camlist.at (0);

        cout << "CamID for slave : " << slaveId << endl;
        slaveCameraDevice = manager.createCamera (slaveId, royale::TriggerMode::SLAVE);
    }

    // the camera devices should now be available and the CameraManager can be deallocated here

    if (slaveCameraDevice == nullptr)
    {
        cerr << "Cannot create the camera device" << endl;
        return 1;
    }

    if (slaveCameraDevice->initialize() != royale::CameraStatus::SUCCESS)
    {
        cerr << "Cannot initialize the slave" << endl;
        return 1;
    }

    // register the data listener
    if (slaveCameraDevice->registerDataListener (&slaveListener) != royale::CameraStatus::SUCCESS)
    {
        cerr << "Error registering data listener for the slave" << endl;
        return 1;
    }

    // retrieve available use cases
    royale::Vector<royale::String> useCasesSlave;
    if (slaveCameraDevice->getUseCases (useCasesSlave) != royale::CameraStatus::SUCCESS)
    {
        cerr << "Error retrieving use cases for the slave" << endl;
        return 1;
    }

    // choose a use case without mixed mode
    auto selectedUseCaseIdx = 0u;
    auto useCaseFound = false;
    for (auto i = 0u; i < useCasesSlave.size(); ++i)
    {
        uint32_t streamCount = 0;
        if (slaveCameraDevice->getNumberOfStreams (useCasesSlave[i], streamCount) != royale::CameraStatus::SUCCESS)
        {
            cerr << "Error retrieving the number of streams for use case " << useCasesSlave[i] << endl;
            return 1;
        }

        if (streamCount == 1)
        {
            // we found a use case with only one stream
            selectedUseCaseIdx = i;
            useCaseFound = true;
            break;
        }
    }

    // check if we found a suitable use case
    if (!useCaseFound)
    {
        cerr << "Error : There are only mixed modes available" << endl;
        return 1;
    }

    const auto useCase = useCasesSlave.at (selectedUseCaseIdx);

    // set an appropriate use case (in this case we take the first one)
    if (slaveCameraDevice->setUseCase (useCase) != royale::CameraStatus::SUCCESS)
    {
        cerr << "Error setting use case '" << useCase << "' for the slave" << endl;
        return 1;
    }

    // start capturing
    if (slaveCameraDevice->startCapture() != royale::CameraStatus::SUCCESS)
    {
        cerr << "Error starting the capturing for the slave" << endl;
        return 1;
    }


    // let the cameras capture for some time
    this_thread::sleep_for (chrono::seconds (5));

    // stop capturing
    if (slaveCameraDevice->stopCapture() != royale::CameraStatus::SUCCESS)
    {
        cerr << "Error stopping the capturing for the slave" << endl;
        return 1;
    }

    return 0;
}
