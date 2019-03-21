/****************************************************************************\
* Copyright (C) 2017 pmdtechnologies ag
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
#include <chrono>

#include <sample_utils/PlatformResources.hpp>

#ifndef ROYALE_ACCESS_CODE_LEVEL2
#define ROYALE_ACCESS_CODE_LEVEL2 "" // Insert activation code here
#endif

using namespace sample_utils;
using namespace std;

class MyListener : public royale::IDepthDataListener
{
    void onNewData (const royale::DepthData *data) override
    {
        // do something with the data
    }
};

int main()
{
    // Windows requires that the application allocate these, not the DLL.
    PlatformResources resources;

    // This is the data listener which will receive callbacks.  It's declared
    // before the cameraDevice so that, if this function exits with a 'return'
    // statement while the camera is still capturing, it will still be in scope
    // until the cameraDevice's destructor implicitly deregisters the listener.
    MyListener listener;

    // Check if we have the appropriate access level
    // (the following operations need Level 2 access)
    if (royale::CameraManager::getAccessLevel (ROYALE_ACCESS_CODE_LEVEL2) < royale::CameraAccessLevel::L2)
    {
        cerr << "Please insert the activation code for Level 2 into the define at the beginning of this program!" << endl;
        return 1;
    }

    // this represents the main camera device object
    unique_ptr<royale::ICameraDevice> cameraDevice;

    // the camera manager will query for a connected camera
    {
        royale::CameraManager manager (ROYALE_ACCESS_CODE_LEVEL2);

        auto camlist = manager.getConnectedCameraList();
        cout << "Detected " << camlist.size() << " camera(s)." << endl;
        if (!camlist.empty())
        {
            cout << "CamID for first device: " << camlist.at (0).c_str() << " with a length of (" << camlist.at (0).length() << ")" << endl;
            cameraDevice = manager.createCamera (camlist[0]);
        }
    }
    // the camera device is now available and CameraManager can be deallocated here

    if (cameraDevice == nullptr)
    {
        cerr << "Cannot create the camera device" << endl;
        return 1;
    }

    // IMPORTANT: call the initialize method before working with the camera device
    if (cameraDevice->initialize() != royale::CameraStatus::SUCCESS)
    {
        cerr << "Cannot initialize the camera device" << endl;
        return 1;
    }

    royale::Vector<royale::String> useCases;
    auto status = cameraDevice->getUseCases (useCases);

    if (status != royale::CameraStatus::SUCCESS || useCases.empty())
    {
        cerr << "No use cases are available" << endl;
        cerr << "getUseCases() returned: " << getErrorString (status) << endl;
        return 1;
    }

    // register a data listener
    if (cameraDevice->registerDataListener (&listener) != royale::CameraStatus::SUCCESS)
    {
        cerr << "Error registering data listener" << endl;
        return 1;
    }

    // choose a use case without mixed mode
    auto selectedUseCaseIdx = 0u;
    auto useCaseFound = false;
    for (auto i = 0u; i < useCases.size(); ++i)
    {
        uint32_t streamCount = 0;
        if (cameraDevice->getNumberOfStreams (useCases[i], streamCount) != royale::CameraStatus::SUCCESS)
        {
            cerr << "Error retrieving the number of streams for use case " << useCases[i] << endl;
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

    // set an operation mode
    if (cameraDevice->setUseCase (useCases.at (selectedUseCaseIdx)) != royale::CameraStatus::SUCCESS)
    {
        cerr << "Error setting use case" << endl;
        return 1;
    }

    // start capture mode
    if (cameraDevice->startCapture() != royale::CameraStatus::SUCCESS)
    {
        cerr << "Error starting the capturing" << endl;
        return 1;
    }

    // retrieve the current processing parameters
    // (there are two methods for this, one without streamId and one
    // with streamId. as we have chosen a non mixed mode use case
    // we don't need to specify the streamId)
    royale::ProcessingParameterVector currentParameters;
    if (cameraDevice->getProcessingParameters (currentParameters) != royale::CameraStatus::SUCCESS)
    {
        cerr << "Unable to retrieve the current processing parameters" << endl;
        return 1;
    }

    // output the current parameters
    cout << "Current parameters : " << endl;
    for (auto param : currentParameters)
    {
        cout << getProcessingFlagName (param.first).c_str() << " : ";
        switch (param.second.variantType())
        {
            case royale::VariantType::Int:
                {
                    cout << param.second.getInt() << endl;
                    break;
                }
            case royale::VariantType::Bool:
                {
                    if (param.second.getBool())
                    {
                        cout << "True" << endl;
                    }
                    else
                    {
                        cout << "False" << endl;
                    }
                    break;
                }
            case royale::VariantType::Float:
                {
                    cout << param.second.getFloat() << endl;
                    break;
                }
        }
    }

    // let the camera capture for some time
    this_thread::sleep_for (chrono::seconds (5));

    // change a parameter of the processing pipeline.
    // (for an overview of the existing parameters please have a look
    // at the ProcessingFlag.hpp file)
    if (cameraDevice->setProcessingParameters ({ { royale::ProcessingFlag::UseRemoveFlyingPixel_Bool, royale::Variant (false) } })
    != royale::CameraStatus::SUCCESS)
    {
        cerr << "Error changing the parameter" << endl;
        return 1;
    }

    // let the camera capture for some time
    this_thread::sleep_for (chrono::seconds (5));

    // stop capture mode
    if (cameraDevice->stopCapture() != royale::CameraStatus::SUCCESS)
    {
        cerr << "Error stopping the capturing" << endl;
        return 1;
    }

    return 0;
}
