#pragma once

#include <memory>
#include <mutex>
#include <thread>

#include <arf/BulletinBoard.h>
#include <arf/TimeLimitedTemporalBuffer.h>

#include "coretech/common/engine/robotTimeStamp.h"
#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/imageCache.h"
#include "coretech/vision/engine/markerDetector.h"
#include "json/json.h"

namespace Anki {

class SimpleVisionSystem {

  SimpleVisionSystem(ARF::BulletinBoard &b);

  ~SimpleVisionSystem();

private:
  void ProcessingThread();

  std::atomic<bool> stop_processing_;
  std::thread processing_thread_;
  std::unique_ptr<Anki::Vision::FaceTracker> face_tracker_;
  std::unique_ptr<Anki::Vision::MarkerDetector> marker_detector_;
  Anki::Vision::Camera camera_;

  ARF::TopicViewer<Anki::Vision::ImageBuffer> image_buffer_viewer_;
//  using ImageCacheTopicDataType =
//      ARF::TimeLimitedTemporalBuffer<std::shared_ptr<const Vision::ImageCache>,
//                                     RobotTimeStamp_t>;
//  ARF::TopicPoster<ImageCacheTopicDataType> image_cache_poster_;
  using TrackedFacesTopicDataType = ARF::TimeLimitedTemporalBuffer<
      std::shared_ptr<const std::list<Vision::TrackedFace>>, RobotTimeStamp_t>;
  ARF::TopicPoster<TrackedFacesTopicDataType> tracked_faces_poster_;
  using ObservedMarkersTopicDataType = ARF::TimeLimitedTemporalBuffer<
      std::shared_ptr<const std::list<Vision::ObservedMarker>>,
      RobotTimeStamp_t>;
  ARF::TopicPoster<ObservedMarkersTopicDataType> observed_markers_poster_;
};

} // namespace Anki