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

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>

#include <sample_utils/PlatformResources.hpp>

using namespace sample_utils;
using namespace std;

/**
 * A listener which receives a callback to onNewData when each depth frame is captured.
 *
 * In a real application this could be a UI widget which shows data to the user, or an object or
 * gesture detector that updates an internal 3D model.  This example uses basic ascii-art to print
 * low-resolution versions of the images to std::cout, a basic UI widget without dependence on a
 * UI toolkit.
 *
 * This class is written assuming that it will have callbacks from two separate sources, the Royale
 * framework's onNewData() and the UI toolkit's paint(); a lot of complexity is added for passing
 * data in a thread-safe way between the two callbacks.  If your application will only process
 * recorded files, then the sampleExportPLY shows a much simpler IDepthDataListener.
 */
class MyListener : public royale::IDepthDataListener
{
    /**
     * The output is scaled to fit in a terminal of this size. Although this is bigger than the
     * normal default size of 24 lines, a user with a smaller terminal is likely to scroll upwards
     * and understand the data that they're seeing. Limiting it to 24 lines makes the image much
     * harder to understand.
     */
    static const size_t MAX_HEIGHT = 40;
    /**
     * The output is scaled to fit in a terminal of this size. Here a terminal size of at least 80
     * columns is assumed, and a user with a narrower terminal window will get a very confusing
     * picture; but with all the wrapped-round lines being the same length the user will hopefully
     * understand that a larger window is required.
     */
    static const size_t MAX_WIDTH = 76;

    /**
     * Helper function to display the image as basic ASCII art.
     */
    char asciiPoint (const royale::DepthData *data, std::size_t x, std::size_t y)
    {
        // Using a bounds-check here is inefficient, but allows the scale-to-max-width-or-height
        // loop in onNewData to be simpler.
        if (x >= data->width || y >= data->height)
        {
            return ' ';
        }

        auto depth = data->points[y * data->width + x];
        if (depth.depthConfidence < 128)
        {
            return ' ';
        }
        const royale::Vector<royale::Pair<float, char>> DEPTH_LETTER
        {
            {0.25f, 'O'},
            {0.30f, 'o'},
            {0.35f, '+'},
            {0.40f, '-'},
            {1.00f, '.'},
        };
        for (auto x : DEPTH_LETTER)
        {
            if (depth.z < x.first)
            {
                return x.second;
            }
        }
        return ' ';
    }

    /**
     * Data that has been received in onNewData, and will be printed in the paint() method.
     */
    struct MyFrameData
    {
        std::vector<uint32_t> exposureTimes;
        std::vector<std::string> asciiFrame;
    };

public:
    /**
     * This callback is called for each depth frame that is captured.  In a mixed-mode use case
     * (a use case with multiple streams), each callback refers to data from a single stream.
     */
    void onNewData (const royale::DepthData *data) override
    {
        // Demonstration of how to retrieve the exposure times for the current stream. This is a
        // vector which can contain several numbers, because the depth frame is calculated from
        // several individual raw frames.
        auto exposureTimes = data->exposureTimes;

        // The data pointer will become invalid after onNewData returns.  When processing the data,
        // it's necessary to either:
        // 1. Do all the processing before this method returns, or
        // 2. Copy the data (not just the pointer) for later processing, or
        // 3. Do processing that needs the full data here, and generate or copy only the data
        //    required for later processing
        //
        // The Royale library's depth-processing thread may block while waiting for this function to
        // return; if this function is slow then there may be some lag between capture and onNewData
        // for the next frame.  If it's very slow then Royale may drop frames to catch up.
        //
        // This sample code assumes that the UI toolkit will provide a callback to the paint()
        // method as needed, but does the initial processing of the data in the current thread.
        //
        // This sample code uses option 3 above, the conversion from DepthData to MyFrameData is
        // done in this method, and MyFrameData provides all the data required for the paint()
        // method, without needing any pointers to the memory owned by the DepthData instance.
        //
        // The image will also be scaled to the expected width, or may be scaled down to less than
        // the expected width if necessary for the height limitation.
        std::size_t scale = std::max ({size_t (1u), data->width / m_widthPerStream, data->height / MAX_HEIGHT});
        std::size_t height = data->height / scale;

        // To reduce the depth data to ascii art, this sample discards most of the information in
        // the data.  However, even if we had a full GUI, treating the depth data as an array of
        // (data->width * data->height) z coordinates would lose the accuracy.  The 3D depth
        // points are not arranged in a rectilinear projection, and the discarded x and y
        // coordinates from the depth points account for the optical projection (or optical
        // distortion) of the camera.
        std::vector<std::string> asciiFrame;
        for (auto y = 0u; y < height; y++)
        {
            std::string asciiLine;
            asciiLine.reserve (m_widthPerStream);
            for (auto x = 0u; x < m_widthPerStream; x++)
            {
                // There is a bounds-check in the asciiPoint function, it returns a space character
                // if x or y is out-of-bounds.
                asciiLine.push_back (asciiPoint (data, x * scale, y * scale));
            }
            asciiFrame.push_back (std::move (asciiLine));
        }

        // Scope for a lock while updating the internal model
        {
            std::unique_lock<std::mutex> lock (m_lockForReceivedData);
            auto &receivedData = m_receivedData[data->streamId];
            receivedData.asciiFrame.swap (asciiFrame);
            receivedData.exposureTimes = exposureTimes.toStdVector();
        }

        // In a full application, the call below to paint() should be done in a separate thread, as
        // explained in the comment above.  UI toolkits are expected to provide a method to to
        // request an asynchronous repaint of the screen, without blocking onNewData().
        //
        // But for the purposes of a demo, dropped frames are an accepted trade-off for not
        // depending on a specific UI toolkit.
        paint();
    }

    /**
     * In a full application, this would be the graphical widget's method for updating the screen,
     * called from the UI toolkit.
     *
     * It paints the data from all streams, although in this demo it's called each time that each
     * stream updates (so all except one of the streams will look the same as the previous display).
     */
    void paint ()
    {
        // to show multiple streams side-by-size, we temporarily blit them in to this area
        std::vector<std::string> allFrames;
        std::map<royale::StreamId, std::vector<uint32_t>> allExposureTimes;

        // scope for the thread-safety lock
        {
            // while locked, copy the data to local structures
            std::unique_lock<std::mutex> lock (m_lockForReceivedData);
            if (m_receivedData.empty())
            {
                return;
            }
            // allocate the allFrames area, with a set of empty strings
            {
                const auto received = m_receivedData.begin();
                allFrames.resize (received->second.asciiFrame.size());
                for (auto &str : allFrames)
                {
                    str.reserve (m_width);
                }
            }

            // For each stream, append its data to each line of allFrames
            for (auto streamId : m_streamIds)
            {
                const auto received = m_receivedData.find (streamId);
                if (received != m_receivedData.end())
                {
                    for (auto y = 0u; y < allFrames.size(); y++)
                    {
                        // The at() in the next line could throw if streams have different-sized
                        // frames. That situation isn't expected in Royale, so this example omits
                        // error handling for this.
                        const auto &src = received->second.asciiFrame.at (y);
                        auto &dest = allFrames.at (y);
                        dest.append (src.begin(), src.end());
                    }
                    allExposureTimes[streamId] = received->second.exposureTimes;
                }
                else
                {
                    // There's no capture for this stream yet, leave a blank space
                    for (auto &dest : allFrames)
                    {
                        dest.append (m_widthPerStream, ' ');
                    }
                }
            }
        }

        // If this is a mixed mode (with multiple streams), print a header with the StreamIds
        if (m_streamIds.size() > 1)
        {
            for (auto streamId : m_streamIds)
            {
                // streamIds are uint16_ts.  To print them with C++'s `<<` operator they should be
                // converted to unsigned, otherwise they may be interpreted as wide-characters.
                cout << setw (static_cast<int> (m_widthPerStream)) << setfill (' ') << unsigned (streamId);
            }
            cout << endl;
        }
        // Print the data from all of the captured streams
        for (const auto &line : allFrames)
        {
            cout << string (line.data(), line.size()) << endl;
        }

        for (auto streamId : m_streamIds)
        {
            const auto &exposureTimes = allExposureTimes.find (streamId);
            cout << "ExposureTimes for stream[" << static_cast<unsigned> (streamId) << "] : ";
            if (exposureTimes == allExposureTimes.end())
            {
                cout << "no frames received yet for this stream";
            }
            else
            {
                bool firstElementOfList = true;
                for (const auto exposure : exposureTimes->second)
                {
                    if (firstElementOfList)
                    {
                        firstElementOfList = false;
                    }
                    else
                    {
                        cout << ", ";
                    }
                    cout << exposure;
                }
            }
            cout << endl;
        }
    }

    /**
     * Creates a listener which will have callbacks from two sources - the Royale framework, when a
     * new frame is received, and the UI toolkit, when the graphics are ready to repaint.
     */
    explicit MyListener (const royale::Vector<royale::StreamId> &streamIds) :
        m_width (MAX_WIDTH),
        m_widthPerStream (MAX_WIDTH / streamIds.size()),
        m_streamIds (streamIds)
    {
    }

private:
    /**
     * The maximum width of the display, including all of the streams.
     */
    const std::size_t m_width;

    /**
     * How many pixels (or for this example how many characters) each received depth image should be
     * resized to.
     */
    const std::size_t m_widthPerStream;

    /**
     * The StreamIds for all streams that are expected to be received.  For this example, it's a
     * constant set, so doesn't need protecting with a mutex.
     */
    const royale::Vector<royale::StreamId> m_streamIds;

    /**
     * Updated in each call to onNewData, for each stream it contains the most recently received
     * frame.  Should only be accessed with m_lockForReceivedData held.
     */
    std::map<royale::StreamId, MyFrameData> m_receivedData;
    std::mutex m_lockForReceivedData;
};

/**
 * This takes one optional argument, the name of a use case (that doesn't end
 * with .rrf) or the name of a recording (that does end with .rrf).
 */
int main (int argc, char **argv)
{
    // Windows requires that the application allocate these, not the DLL.  We expect typical
    // Royale applications to be using a GUI toolkit such as Qt, which has its own equivalent of this
    // PlatformResources class (automatically set up by the toolkit).
    PlatformResources resources;

    // This is the data listener which will receive callbacks.  It's declared
    // before the cameraDevice so that, if this function exits with a 'return'
    // statement while the camera is still capturing, it will still be in scope
    // until the cameraDevice's destructor implicitly deregisters the listener.
    unique_ptr<MyListener> listener;

    // this represents the main camera device object
    unique_ptr<royale::ICameraDevice> cameraDevice;

    // if non-null, load this file instead of connecting to a camera
    unique_ptr<royale::String> rrfFile;
    // if non-null, choose this use case instead of the default
    unique_ptr<royale::String> commandLineUseCase;

    if (argc > 1)
    {
        // Files recorded with RoyaleViewer have this filename extension
        const auto FILE_EXTENSION = royale::String (".rrf");

        auto arg = std::unique_ptr<royale::String> (new royale::String (argv[1]));
        if (*arg == "--help" || *arg == "-h" || *arg == "/?" || argc > 2)
        {
            cout << argv[0] << ":" << endl;
            cout << "--help, -h, /? : this help" << endl;
            cout << endl;
            cout << "With no command line arguments: opens a camera and retrieves data using the default use case" << endl;
            cout << endl;
            cout << "With an argument that ends \".rrf\": assumes the argument is the filename of a recording, and plays it" << endl;
            cout << "When playing back a recording, the only use case available is the one that was recorded." << endl;
            cout << endl;
            cout << "With an argument that doesn't end \".rrf\": assumes the argument is the name of a use-case" << endl;
            cout << "It will open the camera, and if there's a use case with that exact name will use it." << endl;
            return 0;
        }
        else if (arg->size() > FILE_EXTENSION.size() && 0 == arg->compare (arg->size() - FILE_EXTENSION.size(), FILE_EXTENSION.size(), FILE_EXTENSION))
        {
            cout << "Assuming command-line argument is the filename of an RRF recording" << endl;
            rrfFile = std::move (arg);
        }
        else
        {
            cout << "Assuming command-line argument is the name of a use case, as it does not end \".rrf\"" << endl;
            commandLineUseCase = std::move (arg);
        }
    }

    // the camera manager will either open a recording or query for a connected camera
    if (rrfFile)
    {
        royale::CameraManager manager;
        cameraDevice = manager.createCamera (*rrfFile);
    }
    else
    {
        royale::CameraManager manager;

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

    // choose a use case
    auto selectedUseCaseIdx = 0u;
    if (commandLineUseCase)
    {
        auto useCaseFound = false;

        for (auto i = 0u; i < useCases.size(); ++i)
        {
            if (*commandLineUseCase == useCases[i])
            {
                selectedUseCaseIdx = i;
                useCaseFound = true;
                break;
            }
        }

        if (!useCaseFound)
        {
            cerr << "Error: the chosen use case is not supported by this camera" << endl;
            cerr << "A list of supported use cases is printed by sampleCameraInfo" << endl;
            return 1;
        }
    }
    else
    {
        // choose the first use case
        selectedUseCaseIdx = 0;
    }

    // set an operation mode
    if (cameraDevice->setUseCase (useCases.at (selectedUseCaseIdx)) != royale::CameraStatus::SUCCESS)
    {
        cerr << "Error setting use case" << endl;
        return 1;
    }

    // Retrieve the IDs of the different streams
    royale::Vector<royale::StreamId> streamIds;
    if (cameraDevice->getStreams (streamIds) != royale::CameraStatus::SUCCESS)
    {
        cerr << "Error retrieving streams" << endl;
        return 1;
    }

    cout << "Stream IDs : ";
    for (auto curStream : streamIds)
    {
        cout << curStream << ", ";
    }
    cout << endl;

    // register a data listener
    listener.reset (new MyListener (streamIds));
    if (cameraDevice->registerDataListener (listener.get()) != royale::CameraStatus::SUCCESS)
    {
        cerr << "Error registering data listener" << endl;
        return 1;
    }

    // start capture mode
    if (cameraDevice->startCapture() != royale::CameraStatus::SUCCESS)
    {
        cerr << "Error starting the capturing" << endl;
        return 1;
    }

    // let the camera capture for some time
    this_thread::sleep_for (chrono::seconds (5));

    // Change the exposure time for the first stream of the use case (Royale will limit this to an
    // eye-safe exposure time, with limits defined by the use case).  The time is given in
    // microseconds.
    //
    // Non-mixed mode use cases have exactly one stream, mixed mode use cases have more than one.
    // For this example we only change the first stream.
    if (cameraDevice->setExposureTime (200, streamIds[0]) != royale::CameraStatus::SUCCESS)
    {
        cerr << "Cannot set exposure time for stream" << streamIds[0] << endl;
    }
    else
    {
        cout << "Changed exposure time for stream " << streamIds[0] << " to 200 microseconds ..." << endl;
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
