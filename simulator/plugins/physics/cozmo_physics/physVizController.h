/**
* File: physVizController
*
* Author: damjan stulic
* Created: 9/15/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/
#ifndef __CozmoPhysics_PhysVizControllerImpl_H__
#define __CozmoPhysics_PhysVizControllerImpl_H__

#include "anki/messaging/shared/UdpServer.h"
#include "anki/cozmo/basestation/events/ankiEventMgr.h"
#include "clad/vizInterface/messageViz.h"
#include <vector>
#include <unordered_map>
#include <map>
#define DEBUG_COZMO_PHYSICS 0


namespace Anki {
namespace Cozmo {

class PhysVizController
{

public:
  PhysVizController() {};

  void Init();
  void Update();
  void Draw(int pass, const char *view);
  void Cleanup();
  
private:

  void ProcessMessage(VizInterface::MessageViz&& message);

  void Subscribe(const VizInterface::MessageVizTag& tagType, std::function<void(const AnkiEvent<VizInterface::MessageViz>&)> messageHandler) {
    _eventMgr.SubscribeForever(static_cast<uint32_t>(tagType), messageHandler);
  }

  void ProcessVizObjectMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizQuadMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizEraseObjectMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizEraseQuadMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizAppendPathSegmentLineMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizAppendPathSegmentArcMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizSetPathColorMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizErasePathMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizDefineColorMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizShowObjectsMessage(const AnkiEvent<VizInterface::MessageViz>& msg);
  void ProcessVizSetOriginMessage(const AnkiEvent<VizInterface::MessageViz>& msg);

  void DrawTextAtOffset(std::string s, float x_off, float y_off, float z_off);
  void DrawCuboid(float x_dim, float y_dim, float z_dim);
  void DrawRamp(float platformLength, float slopeLength, float width, float height);
  void DrawHead(float width, float height, float depth);
  void DrawTetrahedronMarker(const float x, const float y, const float z,
    const float length_x, const float length_y, const float length_z);
  void DrawRobot(Anki::Cozmo::VizRobotMarkerType type);
  void DrawPredockPose();
  void DrawQuad(const float xUpperLeft,  const float yUpperLeft, const float zUpperLeft,
    const float xLowerLeft,  const float yLowerLeft, const float zLowerLeft,
    const float xUpperRight, const float yUpperRight, const float zUpperRight,
    const float xLowerRight, const float yLowerRight, const float zLowerRight);


  AnkiEventMgr<VizInterface::MessageViz> _eventMgr;



  // Types for paths
  //using PathVertex_t = std::vector<float>;
  //using Path_t = std::vector<PathVertex_t>;
  //using PathMap_t = std::unordered_map<uint32_t, Path_t >;
  //
  //// Map of all paths indexed by robotID and pathID
  //PathMap_t pathMap_;
  std::unordered_map<uint32_t, std::vector<std::vector<float> > > _pathMap;

  // Map of pathID to colorID
  std::unordered_map<uint32_t, uint32_t> _pathColorMap;

  // objects
  //using VizObject_t = std::unordered_map<uint32_t, VizInterface::Object>;
  //VizObject_t objectMap_;
  std::map<uint32_t, VizInterface::Object> _objectMap;

  // quads
  //using VizQuadMap_t = std::unordered_map<uint32_t, VizInterface::Quad>;
  //using VizQuadTypeMap_t = std::unordered_map<uint32_t, VizQuadMap_t>;
  //VizQuadTypeMap_t quadMap_;
  std::unordered_map<uint32_t, std::unordered_map<uint32_t, VizInterface::Quad> > _quadMap;

  // Color map
  //using VizColorDef_t = std::unordered_map<uint32_t, VizInterface::DefineColor>;
  //VizColorDef_t colorMap_;
  std::unordered_map<uint32_t, VizInterface::DefineColor> _colorMap;

  // Server that listens for visualization messages from basestation's VizManger
  UdpServer _server;

  // Whether or not to draw anything
  bool _drawEnabled = true;

  // Default height offset of paths (m)
  float _heightOffset = 0.045;

  // Default angular resolution of arc path segments (radians)
  float _arcRes_rad = 0.2;

  // Global offset
  float _globalRotation[4] = {0,0,0,0};    // angle, axis_x, axis_y, axis_z
  float _globalTranslation[3] = {0, 0, 0}; // x,y,z

};

} // end namespace Cozmo
} // end namespace Anki



#endif //__CozmoPhysics_PhysVizControllerImpl_H__
