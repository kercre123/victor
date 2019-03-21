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
#include <sample_utils/PlatformResources.hpp>

#include <condition_variable>
#include <mutex>
#include <memory>
#include <iostream>
#include <cstdint>

// This is a standard implementation for handling an error on the camera device.
//
// In other applications it might be a better idea to retry some of the methods
// but for this sample this should be enough.
#define CHECKED_CAMERA_METHOD(METHOD_TO_INVOKE)\
do\
{\
    auto status = METHOD_TO_INVOKE;\
    if (royale::CameraStatus::SUCCESS != status)\
    {\
        std::cout << royale::getStatusString(status).c_str() << std::endl\
            << "Press Enter to exit the application ...";\
        \
        return -1;\
    }\
} while (0)

namespace
{
    /**
     * This variables are used to ensure that the
     * main method is open until the camera has stopped capture.
     */
    std::mutex mtx;
    std::condition_variable condition;
    bool notified = false;

    /**
     * The MyRecordListener waits for the camera device to close.
     * Then the MyRocordListener will tidy up the camera device.
     */
    class MyRecordListener : public royale::IRecordStopListener
    {
    public:

        void onRecordingStopped (const uint32_t numFrames) override
        {
            std::cout << "The onRecordingStopped was invoked with numFrames=" << numFrames << std::endl;

            // Notify the main method to return
            std::unique_lock<std::mutex> lock (mtx);
            notified = true;
            condition.notify_all();
        }
    };

    MyRecordListener myRecordListener;
}

/**
* This main method will parse the passed commandline arguments
* and start a camera recording using the parsed parameters.
*/
int main (int argc, char **argv)
{
    // Receive the parameters to capture from the command line:
    if (2 > argc)
    {
        std::cout << "There are no parameters specified! Use:" << std::endl
                  << argv[0] << " C:/path/to/file.rrf [numberOfFrames [framesToSkip [msToSkip]]]" << std::endl;

        return -1;
    }

    royale::String file (argv[1]);
    auto numberOfFrames = argc >= 3 ? std::stoi (argv[2]) : 0;
    auto framesToSkip = argc >= 4 ? std::stoi (argv[3]) : 0;
    auto msToSkip = argc >= 5 ? std::stoi (argv[4]) : 0;

    // If the first argument was "--help", don't treat that as a filename
    if (file == "--help" || file == "-h" || file == "/?")
    {
        std::cout << argv[0] << ":" << std::endl;
        std::cout << "--help, -h, /? : this help" << std::endl;
        std::cout << std::endl;
        std::cout << argv[0] << " C:/path/to/file.rrf [numberOfFrames [framesToSkip [msToSkip]]]" << std::endl;
        std::cout << "Record to the given RRF file, overwriting it if it already exists" << std::endl;
        return 0;
    }

    // Print the parsed parameters to the command line
    std::cout << "start recording using following parameters:" << std::endl
              << "file=" << file << std::endl
              << "numberOfFrames=" << numberOfFrames << std::endl
              << "framesToSkip=" << framesToSkip << std::endl
              << "msToSkip=" << msToSkip << std::endl;

    // Windows requires that the application allocate these, not the DLL.
    sample_utils::PlatformResources platformResources;

    std::unique_ptr<royale::ICameraDevice> cameraDevice;
    // The camera manager will query for a connected camera.
    {
        royale::CameraManager manager;

        auto connectedCameraList = manager.getConnectedCameraList();
        if (0 >= connectedCameraList.count())
        {
            std::cout << "There is no camera connected!" << std::endl;

            return -1;
        }

        cameraDevice = manager.createCamera (connectedCameraList[0]);
    }
    // The camera device is now available and CameraManager can be deallocated here.

    if (nullptr == cameraDevice)
    {
        std::cout << "The camera can not be created!" << std::endl;

        return -1;
    }

    // IMPORTANT: call the initialize method before working with the camera device
    CHECKED_CAMERA_METHOD (cameraDevice->initialize());

    // If the user specified a limited amount of frames to capture
    // we use a listener to tidy up the camera when all frames are captured.
    if (0 < numberOfFrames)
    {
        CHECKED_CAMERA_METHOD (cameraDevice->registerRecordListener (&myRecordListener));

        CHECKED_CAMERA_METHOD (cameraDevice->startCapture());
        CHECKED_CAMERA_METHOD (cameraDevice->startRecording (file, numberOfFrames, framesToSkip, msToSkip));

        // It is important not to close the main method before
        // the record stop callback was received!
        std::unique_lock<std::mutex> lock (mtx);

        // loop to avoid spurious wakeups
        while (!notified)
        {
            condition.wait (lock);
        }

        auto status = cameraDevice->stopCapture();
        if (royale::CameraStatus::SUCCESS != status)
        {
            std::cout << "Failed to close the camera device with status="
                      << royale::getStatusString (status).c_str()
                      << std::endl;
        }
    }
    // If the user does not specified a limited amount of frames to capture
    // we use an user input to recognize when the user want to stop capture.
    else
    {
        CHECKED_CAMERA_METHOD (cameraDevice->startCapture());
        CHECKED_CAMERA_METHOD (cameraDevice->startRecording (file, numberOfFrames, framesToSkip, msToSkip));

        std::cout << "Press Enter to stop capture ..." << std::endl;
        std::cin.get();

        CHECKED_CAMERA_METHOD (cameraDevice->stopRecording());
        CHECKED_CAMERA_METHOD (cameraDevice->stopCapture());
    }

    return 0;
}
