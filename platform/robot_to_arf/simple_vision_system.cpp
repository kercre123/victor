#include "simple_vision_system.h"

#include <fstream>

namespace Anki {

namespace {
static const char kDataPath[] =
    "/anki/data/assets/cozmo_resources/config/engine/vision/";
static const char kJsonFilename[] =
    "/anki/data/assets/cozmo_resources/config/engine/vision_config.json";
} // namespace

SimpleVisionSystem::SimpleVisionSystem(ARF::BulletinBoard &b) {
  const std::array<f32, 8> distortionCoeffs = {
      {-0.03822904514363595, -0.2964213946476391, -0.00181089972406104,
       0.001866070303033584, 0.1803429725181202, 0, 0, 0}};
  auto calib = std::make_shared<Anki::Vision::CameraCalibration>(
      360, 640, 364.7223064012286, 366.1693698832141, 310.6264440545544,
      196.6729350209868, 0, distortionCoeffs);

  camera_.SetCalibration(calib);

  std::ifstream jsonFile(kJsonFilename);
  Json::Reader reader;
  Json::Value jsonData;
  bool success = reader.parse(jsonFile, jsonData);
  if (!success) {
    printf("Problem\n");
  }
  jsonFile.close();

  face_tracker_.reset(
      new Anki::Vision::FaceTracker(camera_, kDataPath, jsonData));
  marker_detector_.reset(new Anki::Vision::MarkerDetector(camera_));

  image_buffer_viewer_ =
      b.MakeViewer<Anki::Vision::ImageBuffer>("image_buffer");
  image_cache_poster_ = b.MakePoster<ImageCacheTopicDataType>("cached_images");
  tracked_faces_poster_ =
      b.MakePoster<TrackedFacesTopicDataType>("tracked_faces");
  observed_markers_poster_ =
      b.MakePoster<ObservedMarkersTopicDataType>("observed_markers");

  stop_processing_ = false;
  processing_thread_ = std::thread(&SimpleVisionSystem::ProcessingThread, this);
}

void SimpleVisionSystem::ProcessingThread() {
  while (!stop_processing_) {
    std::shared_ptr<Anki::Vision::ImageBuffer> image_buffer;
    if (!image_buffer_viewer_.Retrieve(&image_buffer)) {
      usleep(50000);
      continue;
    }
    std::shared_ptr<Vision::ImageCache> image_cache =
        std::make_shared<Vision::ImageCache>();
    image_cache->Reset(*image_buffer);

    const Vision::Image gray_image = image_cache->GetGray();
    std::shared_ptr<std::list<Vision::TrackedFace>> tracked_faces;
    std::list<Anki::Vision::UpdatedFaceID> updated_ids_list;
    Anki::Vision::DebugImageList<Anki::Vision::CompressedImage> debug_images;
    Anki::Result face_tracker_->Update(gray_image, 1.f, tracked_faces,
                                       updated_ids_list, debug_images);
    if (result == 0) {
        tracked_faces_poster_.InvokePost([](TrackedFacesTopicData& buffer){
            
        });
    }
  }
}

} // namespace Anki
