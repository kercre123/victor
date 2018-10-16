/**
* File: vizControllerImpl
*
* Author: damjan stulic
* Created: 9/15/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/
#ifndef __WebotsCtrlViz_VizControllerImpl_H__
#define __WebotsCtrlViz_VizControllerImpl_H__


#include "engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior_fwd.h"
#include "engine/encodedImage.h"
#include "engine/events/ankiEventMgr.h"

#include "coretech/common/engine/robotTimeStamp.h"
#include "coretech/vision/engine/image.h"

#include "util/container/circularBuffer.h"

#include "clad/types/cameraParams.h"
#include "clad/types/emotionTypes.h"
#include "clad/vizInterface/messageViz.h"

#include <webots/Supervisor.hpp>
#include <webots/ImageRef.hpp>
#include <webots/Display.hpp>
#include <vector>
#include <map>

namespace Anki {
namespace Vector {

struct CozmoBotVizParams
{
  webots::Supervisor* supNode = nullptr;
  webots::Field* trans = nullptr;
  webots::Field* rot = nullptr;
  webots::Field* liftAngle = nullptr;
  webots::Field* headAngle = nullptr;
};

// Note: The values of these labels are used to determine the line number
//       at which the corresponding text is displayed in the window.
enum class VizTextLabelType : unsigned int
{
  TEXT_LABEL_POSE = 0,
  TEXT_LABEL_HEAD_LIFT,
  TEXT_LABEL_PITCH,
  TEXT_LABEL_ACCEL,
  TEXT_LABEL_GYRO,
  TEXT_LABEL_CLIFF,
  TEXT_LABEL_DIST,
  TEXT_LABEL_SPEEDS,
  TEXT_LABEL_OFF_TREADS_STATE,
  TEXT_LABEL_TOUCH,
  TEXT_LABEL_BATTERY,
  TEXT_LABEL_ANIM,
  TEXT_LABEL_ANIM_TRACK_LOCKS,
  TEXT_LABEL_VID_RATE,
  TEXT_LABEL_STATUS_FLAG,
  TEXT_LABEL_STATUS_FLAG_2,
  TEXT_LABEL_STATUS_FLAG_3,
  TEXT_LABEL_DOCK_ERROR_SIGNAL,
  NUM_TEXT_LABELS
};


class VizControllerImpl
{

public:
  VizControllerImpl(webots::Supervisor& vs);

  void Init();
  
  void ProcessMessage(VizInterface::MessageViz&& message);

private:

  void SetRobotPose(CozmoBotVizParams *p,
    const f32 x, const f32 y, const f32 z,
    const f32 rot_axis_x, const f32 rot_axis_y, const f32 rot_axis_z, const f32 rot_rad,
    const f32 headAngle, const f32 liftAngle);

  void DrawText(webots::Display* disp, u32 lineNum, u32 color, const char* text);
  void DrawText(webots::Display* disp, u32 lineNum, const char* text);
  void ProcessVizSetRobotMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizSetLabelMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizDockingErrorSignalMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizVisionMarkerMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizCameraQuadMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizCameraRectMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizCameraLineMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizCameraOvalMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizCameraTextMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizImageChunkMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizTrackerQuadMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizRobotStateMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizCurrentAnimation(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessCameraParams(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessBehaviorStackDebug(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVisionModeDebug(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessEnabledVisionModes(const AnkiEvent<VizInterface::MessageViz>& msg);
  
  bool IsMoodDisplayEnabled() const;
  void ProcessVizRobotMoodMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  
  void ProcessSaveImages(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessSaveState(const AnkiEvent<VizInterface::MessageViz>& msg);
  
  void DisplayBufferedCameraImage(const RobotTimeStamp_t timestamp);
  void DisplayCameraInfo(const RobotTimeStamp_t timestamp);
  
  using EmotionBuffer = Util::CircularBuffer<float>;
  using EmotionEventBuffer = Util::CircularBuffer< std::vector<std::string> >;
  
  using CubeAccelBuffer = Util::CircularBuffer<float>;
  
  void Subscribe(const VizInterface::MessageVizTag& tagType, std::function<void(const AnkiEvent<VizInterface::MessageViz>&)> messageHandler) {
    _eventMgr.SubscribeForever(static_cast<uint32_t>(tagType), messageHandler);
  }

  webots::Supervisor& _vizSupervisor;

  // For displaying misc debug data
  webots::Display* _disp;

  // For displaying docking data
  webots::Display* _dockDisp;
  
  // For displaying mood data
  webots::Display* _moodDisp;

  // For the behavior stack
  webots::Display* _bsmStackDisp;

  // For displaying active VisionMode data
  webots::Display* _visionModeDisp;

  // For displaying images
  webots::Display* _camDisp;

  // Image reference for display in camDisp
  webots::ImageRef* _camImg = nullptr;

  // Cozmo bots for visualization

  // Vector of available CozmoBots for vizualization
  std::vector<CozmoBotVizParams> vizBots_;

  // Map of robotID to vizBot index
  std::map<uint8_t, uint8_t> robotIDToVizBotIdxMap_;

  // Image message processing
  static const size_t kNumBufferedImages = 10;
  std::array<EncodedImage, kNumBufferedImages> _bufferedImages;
  size_t                                       _imageBufferIndex = 0;
  std::map<RobotTimeStamp_t, size_t>           _encodedImages;
  std::map<RobotTimeStamp_t, u32>              _bufferedSaveCtrs;
  RobotTimeStamp_t _curImageTimestamp = 0;
  ImageSendMode _saveImageMode = ImageSendMode::Off;
  std::string   _savedImagesFolder = "";
  u32           _saveCtr = 0;
  bool          _saveVizImage = false;
  
  // For managing "debug" image displays
  struct DebugImage {
    EncodedImage      encodedImage;
    webots::Display*  imageDisplay;
    webots::ImageRef* imageRef;
    
    DebugImage(webots::Display* display) : imageDisplay(display), imageRef(nullptr) { }
  };
  std::vector<DebugImage> _debugImages;
  
  // Camera info
  Vision::CameraParams _cameraParams;
  
  // For saving state
  bool          _saveState = false;
  std::string   _savedStateFolder = "";
  
  AnkiEventMgr<VizInterface::MessageViz> _eventMgr;
  
  // Circular buffers of data to show last N ticks of a value
  EmotionBuffer           _emotionBuffers[(size_t)EmotionType::Count];
  EmotionEventBuffer      _emotionEventBuffer;

  std::string _currAnimName = "";
  u8          _currAnimTag = 0;
};

} // end namespace Vector
} // end namespace Anki


#endif //__WebotsCtrlViz_VizControllerImpl_H__
