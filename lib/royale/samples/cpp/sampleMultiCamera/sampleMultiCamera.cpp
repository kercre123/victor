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
#include <iostream>
#include <thread>

#include <sample_utils/PlatformResources.hpp>

using namespace sample_utils;
using namespace std;

namespace
{
    // simple class with an id member to distinguish between the devices
    class CamListener : public royale::IDepthDataListener
    {
    public:

        CamListener() : camID_ ("NOT INITIALIZED") {}
        CamListener (royale::String inId) : camID_ (inId) {}

        royale::String getCamID()
        {
            return camID_;
        }

        void onNewData (const royale::DepthData *data) override
        {
            cout << "Received data for the camera with the id '" << getCamID() << "'..." << endl;

            // Do something with the data
        }

        void setCamID (royale::String inId)
        {
            camID_ = inId;
        }

    private:

        royale::String camID_;
    };

}

/**
 * This example shows how to open and use multiple cameras. It tries to open the cameras and
 * receive the data simultaneously.
 */
int main (int argc, char **argv)
{
    // Windows requires that the application allocate these, not the DLL.
    PlatformResources resources;

    // the data listener which will receive callbacks.
    royale::Vector<CamListener> camListenerVec;

    // represent the main camera device objects
    royale::Vector< unique_ptr<royale::ICameraDevice> > camCameraDeviceVec;

    std::size_t camCount = 0;
    // the camera manager will query for a connected camera
    {
        royale::CameraManager manager;

        auto camlist = manager.getConnectedCameraList();
        camCount = camlist.size();
        cout << "Detected " << camCount << " camera(s)." << endl;

        if (camCount < 2u)
        {
            cerr << "For this example at least two camera devices are needed. Please check the devices or the connection!" << endl;
            return 1;
        }

        // create the camera devices
        for (auto i = 0u; i < camCount; i++)
        {
            cout << "CamID for the " << i + 1 << ". camera is : " << camlist.at (i) << endl;
            unique_ptr<royale::ICameraDevice> camCameraDevice (manager.createCamera (camlist.at (i)));
            camCameraDeviceVec.push_back (std::move (camCameraDevice));
            camListenerVec.push_back (CamListener (camlist.at (i)));
        }
    }


    // the camera devices should now be available and the CameraManager can be deallocated here
    for (auto i = 0u; i < camCount; i++)
    {
        // check if a device was created
        if (camCameraDeviceVec[i] == nullptr)
        {
            cerr << "Cannot create the camera device" << endl;
            return 1;
        }

        // initialize the camera
        if (camCameraDeviceVec[i]->initialize() != royale::CameraStatus::SUCCESS)
        {
            cerr << "Cannot initialize the " << i + 1 << " camera" << endl;
            return 1;
        }

        // register the data listener
        if (camCameraDeviceVec[i]->registerDataListener (&camListenerVec[i]) != royale::CameraStatus::SUCCESS)
        {
            cerr << "Error registering data listener for the " << i + 1 << ". camera" << endl;
            return 1;
        }
    }

    // choose a use case without mixed mode
    auto selectedUseCaseIdx = 0u;
    auto useCaseFound = false;

    royale::Vector< royale::Vector<royale::String> > useCasesSetVec;

    for (auto i = 0u; i < camCount; i++)
    {

        // retrieve available use cases
        royale::Vector<royale::String> useCasesSet;
        if (camCameraDeviceVec[i]->getUseCases (useCasesSet) != royale::CameraStatus::SUCCESS)
        {
            cerr << "Error retrieving use cases for the " << i + 1 << ". camera" << endl;
            return 1;
        }

        for (auto j = 0u; j < useCasesSet.size(); j++)
        {
            uint32_t streamCount = 0;
            if (camCameraDeviceVec[i]->getNumberOfStreams (useCasesSet[j], streamCount) != royale::CameraStatus::SUCCESS)
            {
                cerr << "Error retrieving the number of streams for use case " << useCasesSet[j] << endl;
                return 1;
            }

            if (streamCount == 1)
            {
                // we found a use case with only one stream
                selectedUseCaseIdx = j;
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

        const auto useCase = useCasesSet[selectedUseCaseIdx];

        // set an appropriate use case (in this case we take the first one)
        if (camCameraDeviceVec[i]->setUseCase (useCase) != royale::CameraStatus::SUCCESS)
        {
            cerr << "Error setting use case '" << useCase << "' for the camera with the id " << camListenerVec[i].getCamID() << endl;
            return 1;
        }

        useCasesSetVec.push_back (useCasesSet);

    }

    // starting capturing with all cameras
    for (auto i = 0u; i < camCount; i++)
    {
        // start capturing
        if (camCameraDeviceVec[i]->startCapture() != royale::CameraStatus::SUCCESS)
        {
            cerr << "Error starting the capturing for the camera with the id " << camListenerVec[i].getCamID() << "..." << endl;
            return 1;
        }
    }

    // let the cameras capture for some time
    this_thread::sleep_for (chrono::seconds (5));

    // stop the capturing with all cameras
    for (auto i = 0u; i < camCount; i++)
    {
        if (camCameraDeviceVec[i]->stopCapture() != royale::CameraStatus::SUCCESS)
        {
            cerr << "Error stopping the capturing for the camera with the id " << camListenerVec[i].getCamID() << "..." << endl;
            return 1;
        }
    }

    return 0;
}
