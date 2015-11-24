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

#include "clad/vizInterface/messageViz.h"
#include "clad/types/emotionTypes.h"
#include "anki/cozmo/basestation/events/ankiEventMgr.h"
#include "anki/vision/basestation/image.h"
#include "anki/cozmo/basestation/imageDeChunker.h"
#include "util/container/circularBuffer.h"
#include <webots/Supervisor.hpp>
#include <webots/ImageRef.hpp>
#include <webots/Display.hpp>
#include <vector>

namespace Anki {
namespace Cozmo {

struct CozmoBotVizParams
{
  webots::Supervisor* supNode = nullptr;
  webots::Field* trans = nullptr;
  webots::Field* rot = nullptr;
  webots::Field* liftAngle = nullptr;
  webots::Field* headAngle = nullptr;
};

enum class VizTextLabelType
{
  TEXT_LABEL_POSE,
  TEXT_LABEL_HEAD_LIFT,
  TEXT_LABEL_IMU,
  TEXT_LABEL_SPEEDS,
  TEXT_LABEL_PROX_SENSORS,
  TEXT_LABEL_BATTERY,
  TEXT_LABEL_VID_RATE,
  TEXT_LABEL_ANIM_BUFFER,
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

  void DrawText(VizTextLabelType labelID, u32 color, const char* text);
  void DrawText(VizTextLabelType labelID, const char* text);
  void ProcessVizSetRobotMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizSetLabelMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizDockingErrorSignalMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizVisionMarkerMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizCameraQuadMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizCameraLineMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizCameraOvalMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizCameraTextMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizImageChunkMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizTrackerQuadMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizRobotStateMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  
  bool IsMoodDisplayEnabled() const;
  void ProcessVizRobotMoodMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  
  bool IsBehaviorDisplayEnabled() const;
  void PreUpdateBehaviorDisplay();
  void ProcessVizRobotBehaviorSelectDataMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizNewBehaviorSelectedMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void DrawBehaviorDisplay();
  
  void ProcessVizStartRobotUpdate(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizEndRobotUpdate(const AnkiEvent<VizInterface::MessageViz>& msg);
  
  using EmotionBuffer = Util::CircularBuffer<float>;
  using EmotionEventBuffer = Util::CircularBuffer< std::vector<std::string> >;
  using BehaviorScoreBuffer = Util::CircularBuffer<float>;
  using BehaviorScoreBufferMap = std::map<std::string, BehaviorScoreBuffer>;
  using BehaviorEventBuffer = Util::CircularBuffer< std::vector<std::string> >;
  
  BehaviorScoreBuffer& FindOrAddScoreBuffer(const std::string& inName);

  void Subscribe(const VizInterface::MessageVizTag& tagType, std::function<void(const AnkiEvent<VizInterface::MessageViz>&)> messageHandler) {
    _eventMgr.SubscribeForever(static_cast<uint32_t>(tagType), messageHandler);
  }

  webots::Supervisor& vizSupervisor;

  // For displaying misc debug data
  webots::Display* disp;

  // For displaying docking data
  webots::Display* dockDisp;
  
  // For displaying mood data
  webots::Display* moodDisp;
  
  // For displaying behavior selection data
  webots::Display* behaviorDisp;

  // For displaying images
  webots::Display* camDisp;

  // Image reference for display in camDisp
  webots::ImageRef* camImg = nullptr;

  // Cozmo bots for visualization

  // Vector of available CozmoBots for vizualization
  std::vector<CozmoBotVizParams> vizBots_;

  // Map of robotID to vizBot index
  std::map<uint8_t, uint8_t> robotIDToVizBotIdxMap_;

  // Image message processing
  ImageDeChunker _imageDeChunker;

  AnkiEventMgr<VizInterface::MessageViz> _eventMgr;
  
  // Circular buffers of data to show last N ticks of a value
  EmotionBuffer           _emotionBuffers[(size_t)EmotionType::Count];
  EmotionEventBuffer      _emotionEventBuffer;
  BehaviorScoreBufferMap  _behaviorScoreBuffers;
  BehaviorEventBuffer     _behaviorEventBuffer;
};

} // end namespace Cozmo
} // end namespace Anki


#endif //__WebotsCtrlViz_VizControllerImpl_H__
