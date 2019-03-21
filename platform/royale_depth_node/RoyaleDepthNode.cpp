#include <stdio.h>
#include <unistd.h>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <thread>

#include "generated/proto/arf/ArfMessage.pb.h"
#include "generated/proto/arf/DepthData.pb.h"
#include "arf/Intraprocess.h"
#include "arf/arf.h"

#include <royale.hpp>

using namespace std;

int gUseCase = 3;
bool gRotate = false;

unique_ptr<class MyListener> listener;

// this represents the main camera device object
std::unique_ptr<royale::ICameraDevice> cameraDevice;

class MyListener : public royale::IDepthDataListener {
  struct MyFrameData {
    std::vector<uint32_t> exposureTimes;
    std::vector<std::string> asciiFrame;
  };

 public:
  void onNewData(const royale::DepthData* data) override {
    auto exposureTimes = data->exposureTimes;

    arf_proto::ArfMessage arf_message;
    arf_proto::DepthData* depth_data = arf_message.mutable_depth_data();

    depth_data->mutable_header()->set_time(
        std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
    depth_data->set_width(data->width);
    depth_data->set_height(data->height);
    for (int x = 0; x < data->width; x++) {
      for (int y = 0; y < data->height; y++) {
        auto& point = data->points[y * data->width + x];
        arf_proto::FloatVector3* vec_proto = depth_data->add_data();
        vec_proto->set_x(point.x);
        vec_proto->set_y(point.y);
        vec_proto->set_z(point.z);
      }
    }
    m_pubHandle.publish_string(arf_message.SerializeAsString());
  }

  explicit MyListener(const royale::Vector<royale::StreamId>& streamIds,
                      ARF::PubHandle<std::string>& pubHandle)
      : m_streamIds(streamIds), m_pubHandle(pubHandle) {}

 private:
  const royale::Vector<royale::StreamId> m_streamIds;
  ARF::PubHandle<std::string>& m_pubHandle;

  std::map<royale::StreamId, MyFrameData> m_receivedData;
  std::mutex m_lockForReceivedData;
};

int InitializeCamera(ARF::PubHandle<std::string>& pubHandle) {
  {
    royale::CameraManager manager;

    royale::Vector<royale::String> camlist(manager.getConnectedCameraList());
    cout << "Detected " << camlist.size() << " camera(s)." << endl;

    if (!camlist.empty()) {
      cameraDevice = manager.createCamera(camlist[0]);
    } else {
      cerr << "No suitable camera device detected." << endl
           << "Please make sure that a supported camera is plugged in, all "
              "drivers are "
           << "installed, and you have proper USB permission" << endl;
      return 1;
    }

    camlist.clear();
  }

  if (cameraDevice == nullptr) {
    cerr << "Cannot create the camera device" << endl;
    return 1;
  }

  auto status = cameraDevice->initialize();
  if (status != royale::CameraStatus::SUCCESS) {
    cerr << "Cannot initialize the camera device, error string : "
         << getErrorString(status) << endl;
    return 1;
  }

  royale::String id;
  royale::String name;

  status = cameraDevice->getId(id);
  if (royale::CameraStatus::SUCCESS != status) {
    cerr << "failed to get ID: " << getErrorString(status) << endl;
    return 1;
  }

  royale::Vector<royale::String> useCases;
  status = cameraDevice->getUseCases(useCases);

  for (auto s : useCases) {
    printf("UseCase: %s\n", s.c_str());
  }

  if (status != royale::CameraStatus::SUCCESS || useCases.empty()) {
    cerr << "No use cases are available" << endl;
    cerr << "getUseCases() returned: " << getErrorString(status) << endl;
    return 1;
  }

  unsigned int selectedUseCaseIdx = gUseCase;

  if (cameraDevice->setUseCase(useCases.at(selectedUseCaseIdx)) !=
      royale::CameraStatus::SUCCESS) {
    cerr << "Error setting use case" << endl;
    return 1;
  }

  royale::Vector<royale::StreamId> streamIds;
  if (cameraDevice->getStreams(streamIds) != royale::CameraStatus::SUCCESS) {
    cerr << "Error retrieving streams" << endl;
    return 1;
  }

  cout << "Stream IDs : ";
  for (auto curStream : streamIds) {
    cout << curStream << ", ";
  }
  cout << endl;

  listener.reset(new MyListener(streamIds, pubHandle));
  if (cameraDevice->registerDataListener(listener.get()) !=
      royale::CameraStatus::SUCCESS) {
    cerr << "Error registering data listener" << endl;
    return 1;
  }

  if (cameraDevice->startCapture() != royale::CameraStatus::SUCCESS) {
    cerr << "Error starting the capturing" << endl;
    return 1;
  }

  return 0;
}

int main(int argc, char* argv[]) {
  if (argc < 4) {
    return 1;
  }
  gUseCase = atoi(argv[1]);
  string topic_name = argv[2];
  gRotate = (bool)atoi(argv[3]);

  ARF::Init(argc, argv, "royale_depth_node");
  ARF::NodeHandle n;

  auto pubHandle = n.RegisterPublisher<std::string>(topic_name);

  if (InitializeCamera(pubHandle) != 0) {
    return 1;
  }

  n.Spin();

  return 0;
}
