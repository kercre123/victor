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

#include "physVizController.h"
#include "engine/namedColors/namedColors.h"
#include "engine/viz/vizObjectBaseId.h"
#include "coretech/common/engine/colorRGBA.h"
#include "coretech/common/engine/exceptions.h"
#include "util/math/math.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/vizInterface/messageViz.h"
#include <OpenGL/OpenGL.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#include <plugins/physics.h>
#pragma GCC diagnostic pop
#include <GLUT/GLUT.h>

#if DEBUG_COZMO_PHYSICS
#define PRINT(...) dWebotsConsolePrintf(__VA_ARGS__)
#else
#define PRINT(...)
#endif

namespace Anki {
namespace Vector {


void PhysVizController::Init() {

  _eventMgr.UnsubscribeAll();
  // bind to specific handlers in the robot class
  Subscribe(VizInterface::MessageVizTag::Object,
    std::bind(&PhysVizController::ProcessVizObjectMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::Quad,
    std::bind(&PhysVizController::ProcessVizQuadMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::EraseObject,
    std::bind(&PhysVizController::ProcessVizEraseObjectMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::EraseQuad,
    std::bind(&PhysVizController::ProcessVizEraseQuadMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::AppendPathSegmentLine,
    std::bind(&PhysVizController::ProcessVizAppendPathSegmentLineMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::AppendPathSegmentArc,
    std::bind(&PhysVizController::ProcessVizAppendPathSegmentArcMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::SetPathColor,
    std::bind(&PhysVizController::ProcessVizSetPathColorMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::ErasePath,
    std::bind(&PhysVizController::ProcessVizErasePathMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::DefineColor,
    std::bind(&PhysVizController::ProcessVizDefineColorMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::ShowObjects,
    std::bind(&PhysVizController::ProcessVizShowObjectsMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::SetVizOrigin,
    std::bind(&PhysVizController::ProcessVizSetOriginMessage, this, std::placeholders::_1));
  
  // primitives
  Subscribe(VizInterface::MessageVizTag::SegmentPrimitive,
    std::bind(&PhysVizController::ProcessVizSegmentPrimitiveMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::EraseSegmentPrimitives,
    std::bind(&PhysVizController::ProcessVizEraseSegmentPrimitivesMessage, this, std::placeholders::_1));

  
  Subscribe(VizInterface::MessageVizTag::SimpleQuadVectorMessageBegin,
    std::bind(&PhysVizController::ProcessVizSimpleQuadVectorMessageBegin, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::SimpleQuadVectorMessage,
    std::bind(&PhysVizController::ProcessVizSimpleQuadVectorMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::SimpleQuadVectorMessageEnd,
    std::bind(&PhysVizController::ProcessVizSimpleQuadVectorMessageEnd, this, std::placeholders::_1));

  Subscribe(VizInterface::MessageVizTag::MemoryMapMessageVizBegin,
    std::bind(&PhysVizController::ProcessVizMemoryMapMessageBegin, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::MemoryMapMessageViz,
    std::bind(&PhysVizController::ProcessVizMemoryMapMessage, this, std::placeholders::_1));
  Subscribe(VizInterface::MessageVizTag::MemoryMapMessageVizEnd,
    std::bind(&PhysVizController::ProcessVizMemoryMapMessageEnd, this, std::placeholders::_1));
  

  _server.StopListening();
  _server.StartListening((uint16_t)VizConstants::PHYSICS_PLUGIN_SERVER_PORT);

  _pathMap.clear();

}

void PhysVizController::Update()
{
  const size_t maxPacketSize{(size_t)VizConstants::MaxMessageSize};
  uint8_t data[maxPacketSize]{0};
  ssize_t numBytesRecvd;
  ///// Process messages from basestation /////
  while ((numBytesRecvd = _server.Recv((char*)data, maxPacketSize)) > 0) {
    ProcessMessage(VizInterface::MessageViz(data, (size_t)numBytesRecvd));
  }

}
  
// Draws axes at current point
void DrawAxes(f32 length)
{
  glColor4ub(255, 0, 0, 255);
  glBegin(GL_LINES);
  glVertex3f(0,0,0);
  glVertex3f(length, 0, 0);
  glEnd();
  
  glColor4ub(0, 255, 0, 255);
  glBegin(GL_LINES);
  glVertex3f(0, 0, 0);
  glVertex3f(0, length, 0);
  glEnd();
  
  glColor4ub(0, 0, 255, 255);
  glBegin(GL_LINES);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 0, length);
  glEnd();
}
  

void PhysVizController::Draw(int pass, const char *view)
{

  if (!_drawEnabled) return;

  // Only draw in main 3D view (view == NULL) and not the camera views
  if (pass == 1 && view == NULL) {

    // setup draw style
    glDisable(GL_LIGHTING);
    glLineWidth(2);

    // Set default color
    glColor4ub(::Anki::NamedColors::DEFAULT.r(),
      ::Anki::NamedColors::DEFAULT.g(),
      ::Anki::NamedColors::DEFAULT.b(),
      ::Anki::NamedColors::DEFAULT.alpha());
    
    // Set global offset
    glPushMatrix();
    glTranslatef(_globalTranslation[0], _globalTranslation[1], _globalTranslation[2]);
    glRotatef(_globalRotation[0], _globalRotation[1], _globalRotation[2], _globalRotation[3]);

    // Draw path
    for (auto pathMapIt = _pathMap.begin(); pathMapIt != _pathMap.end(); pathMapIt++) {

      int pathID = pathMapIt->first;

      // Set path color

      if (_pathColorMap.find(pathID) != _pathColorMap.end()) {
        Anki::ColorRGBA pathColor(_pathColorMap[pathID]);
        glColor4ub(pathColor.r(), pathColor.g(), pathColor.b(), pathColor.alpha());
        /*
        VizColorDef_t::iterator cIt = colorMap_.find(pathColorMap_[pathID]);
        if (cIt != colorMap_.end()) {
          Anki::Vector::VizDefineColor *c = &(cIt->second);
          glColor3f(c->r, c->g, c->b);

        }
         */
      }

      // Draw the path
      glBegin(GL_LINE_STRIP);
      for (auto pathIt = pathMapIt->second.begin(); pathIt != pathMapIt->second.end(); pathIt++) {
        glVertex3f(pathIt->x,pathIt->y,pathIt->z + _heightOffset);
      }
      glEnd();

      
      // Draw segment start markers
      glColor4ub(230, 230, 0, 255); // yellow marker
      for (auto pathIt = pathMapIt->second.begin(); pathIt != pathMapIt->second.end(); pathIt++) {
        if (pathIt->isStartOfSegment) {
          glPushMatrix();
          glTranslatef(pathIt->x,pathIt->y,pathIt->z + _heightOffset);
          glutSolidCube(0.001);
          glPopMatrix();
        }
      }
      
      // Restore default color
      //glColor3f(DEFAULT_COLOR[0], DEFAULT_COLOR[1], DEFAULT_COLOR[2]);
      glColor4ub(::Anki::NamedColors::DEFAULT.r(),
        ::Anki::NamedColors::DEFAULT.g(),
        ::Anki::NamedColors::DEFAULT.b(),
        ::Anki::NamedColors::DEFAULT.alpha());
    }


    // Draw objects
    for (auto objIt = _objectMap.begin(); objIt != _objectMap.end(); ++objIt) {

      VizInterface::Object* obj = &(objIt->second);

      // Set color for the block
      Anki::ColorRGBA objColor(obj->color);
      glColor4ub(objColor.r(), objColor.g(), objColor.b(), objColor.alpha());
      /*
      VizColorDef_t::iterator cIt = colorMap_.find(obj->color);
      if (cIt != colorMap_.end()) {
        Anki::Vector::VizDefineColor *c = &(cIt->second);
        glColor3f(c->r, c->g, c->b);
      }
       */

      // Set pose
      glPushMatrix();

      glTranslatef(obj->x_trans_m, obj->y_trans_m, obj->z_trans_m);
      glRotatef(obj->rot_deg, obj->rot_axis_x, obj->rot_axis_y, obj->rot_axis_z);


      // Use objectType-specific drawing functions
      switch(obj->objectTypeID) {
        case VizObjectType::VIZ_OBJECT_ROBOT:
          DrawRobot(VizRobotMarkerType::VIZ_ROBOT_MARKER_SMALL_TRIANGLE);
          break;
        case VizObjectType::VIZ_OBJECT_CUBOID:
        {
          DrawCuboid(obj->x_size_m, obj->y_size_m, obj->z_size_m);

          // Object ID label
          std::string idString = std::to_string(obj->objectID - Anki::Vector::VizObjectBaseID[(int)VizObjectType::VIZ_OBJECT_CUBOID]);
          DrawTextAtOffset(idString, 0.6f*obj->x_size_m, 0.6f*obj->y_size_m, 0.6f*obj->z_size_m);
          DrawTextAtOffset(idString, -0.6f*obj->x_size_m, -0.6f*obj->y_size_m, -0.6f*obj->z_size_m);
          
          break;
        }
          case VizObjectType::VIZ_OBJECT_RAMP:
        {
          float slopeLength = obj->objParameters[0]*obj->x_size_m;
          DrawRamp(obj->x_size_m, slopeLength, obj->y_size_m, obj->z_size_m);
          break;
        }
        case VizObjectType::VIZ_OBJECT_CHARGER:
        {
          // Draw charger
          float slopeLength = obj->objParameters[0]*obj->x_size_m;
          DrawRamp(obj->x_size_m, slopeLength, obj->y_size_m, obj->z_size_m);
          
          // Object ID label
          std::string idString = std::to_string(obj->objectID - Anki::Vector::VizObjectBaseID[(int)VizObjectType::VIZ_OBJECT_CHARGER]);
          DrawTextAtOffset(idString, 0, 0.6f*obj->y_size_m, 0.6f*obj->z_size_m);
          
          break;
        }
        case VizObjectType::VIZ_OBJECT_PREDOCKPOSE:
          DrawPredockPose();
          break;

        case VizObjectType::VIZ_OBJECT_HUMAN_HEAD:
          DrawHead(obj->x_size_m, obj->y_size_m, obj->z_size_m);
          break;

        default:
          PRINT("Unknown objectTypeID %d\n", obj->objectTypeID);
          break;
      }

      DrawAxes(0.005f);
      
      glFlush();

      glPopMatrix();

      // Restore default color
      //glColor3f(DEFAULT_COLOR[0], DEFAULT_COLOR[1], DEFAULT_COLOR[2]);
      glColor4ub(::Anki::NamedColors::DEFAULT.r(),
        ::Anki::NamedColors::DEFAULT.g(),
        ::Anki::NamedColors::DEFAULT.b(),
        ::Anki::NamedColors::DEFAULT.alpha());

    } // for each object

    // Draw quads
    for(const auto & quadsByType : _quadMap) {
      for(const auto & quadsByID : quadsByType.second) {

        const VizInterface::Quad& quad = quadsByID.second;

        // Set color for the quad
        Anki::ColorRGBA quadColor(quad.color);
        glColor4ub(quadColor.r(), quadColor.g(), quadColor.b(), quadColor.alpha());
        /*
        VizColorDef_t::iterator cIt = colorMap_.find(quad.color);
        if (cIt != colorMap_.end()) {
          Anki::Vector::VizDefineColor *c = &(cIt->second);
          glColor3f(c->r, c->g, c->b);
        }
         */

        DrawQuad(quad.xUpperLeft,  quad.yUpperLeft,  quad.zUpperLeft,
          quad.xLowerLeft,  quad.yLowerLeft,  quad.zLowerLeft,
          quad.xUpperRight, quad.yUpperRight, quad.zUpperRight,
          quad.xLowerRight, quad.yLowerRight, quad.zLowerRight);

        // Restore default color
        //glColor3f(DEFAULT_COLOR[0], DEFAULT_COLOR[1], DEFAULT_COLOR[2]);
        glColor4ub(::Anki::NamedColors::DEFAULT.r(),
          ::Anki::NamedColors::DEFAULT.g(),
          ::Anki::NamedColors::DEFAULT.b(),
          ::Anki::NamedColors::DEFAULT.alpha());
      } // for each quad
    } // for each quad type

    // Draw simple quad vectors
    for(const auto & quadVectorPerIdentifier : _simpleQuadVectorMapReady) {
      for(const auto & quadInVector : quadVectorPerIdentifier.second) {

        // Set color for the quad
        Anki::ColorRGBA quadColor(quadInVector.color);
        glColor4ub(quadColor.r(), quadColor.g(), quadColor.b(), quadColor.alpha());
        
        const float halfSize = quadInVector.sideSize*0.5f;
        DrawQuadFill(
            quadInVector.center[0]+halfSize, quadInVector.center[1]+halfSize, quadInVector.center[2], // up L
            quadInVector.center[0]-halfSize, quadInVector.center[1]+halfSize, quadInVector.center[2], // lo L
            quadInVector.center[0]+halfSize, quadInVector.center[1]-halfSize, quadInVector.center[2], // up R
            quadInVector.center[0]-halfSize, quadInVector.center[1]-halfSize, quadInVector.center[2]  // lo R
         );

        const bool kRenderQuadBorder = true;
        if ( kRenderQuadBorder ) {
          glColor4ub(255,255,255,10);
          DrawQuad(
              quadInVector.center[0]+halfSize, quadInVector.center[1]+halfSize, quadInVector.center[2], // up L
              quadInVector.center[0]-halfSize, quadInVector.center[1]+halfSize, quadInVector.center[2], // lo L
              quadInVector.center[0]+halfSize, quadInVector.center[1]-halfSize, quadInVector.center[2], // up R
              quadInVector.center[0]-halfSize, quadInVector.center[1]-halfSize, quadInVector.center[2]  // lo R
          );
        }
      } // for each quadInVector
    } // for each vector in map
    
    // Draw segment primitives
    glBegin(GL_LINES);
    for ( const auto& segmentVectorPerIdentifier : _segmentPrimitives )
    {
      for ( const auto& segmentInVector : segmentVectorPerIdentifier.second )
      {
        // Set color for the segment
        Anki::ColorRGBA segmentColor(segmentInVector.color);
        glColor4ub(segmentColor.r(), segmentColor.g(), segmentColor.b(), segmentColor.alpha());
        
        // draw segment
        glVertex3f(segmentInVector.origin[0], segmentInVector.origin[1], segmentInVector.origin[2] );
        glVertex3f(segmentInVector.dest[0], segmentInVector.dest[1], segmentInVector.dest[2]);
      }
    }
    glEnd();

    // Restore default color
    glColor4ub(::Anki::NamedColors::DEFAULT.r(),
      ::Anki::NamedColors::DEFAULT.g(),
      ::Anki::NamedColors::DEFAULT.b(),
      ::Anki::NamedColors::DEFAULT.alpha());

    glPopMatrix(); // global viz transform
    
  } // if (pass == 1 && view == NULL)
} // Draw()


void PhysVizController::Cleanup()
{
  _server.StopListening();
  _pathMap.clear();
  _pathColorMap.clear();
  _objectMap.clear();
  _quadMap.clear();
  _colorMap.clear();
  _simpleQuadVectorMapReady.clear();
  _simpleQuadVectorMapIncoming.clear();
}

void PhysVizController::ProcessMessage(VizInterface::MessageViz&& message)
{
  PRINT( "Processing msgs from Basestation: Got msg %s\n", VizInterface::MessageVizTagToString(message.GetTag()));
  uint32_t type = static_cast<uint32_t>(message.GetTag());
  _eventMgr.Broadcast(AnkiEvent<VizInterface::MessageViz>(
    type, std::move(message)));
}

void PhysVizController::ProcessVizObjectMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_Object();
  PRINT("Processing DrawObject %d %d (%f %f %f) (%f %f %f %f) %d \n",
    payload.objectID,
    payload.objectTypeID,
    payload.x_trans_m, payload.y_trans_m, payload.z_trans_m,
    payload.rot_deg, payload.rot_axis_x, payload.rot_axis_y, payload.rot_axis_z,
    payload.color);

  _objectMap[payload.objectID] = VizInterface::Object(payload);
}

void PhysVizController::ProcessVizSegmentPrimitiveMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_SegmentPrimitive();
  PRINT("Processing SegmentPrimitive (%s)\n", payload.identifier.c_str());
  
  if ( payload.clearPrevious ) {
    _segmentPrimitives[payload.identifier].clear();
  }
  
  _segmentPrimitives[payload.identifier].emplace_back( payload.color, payload.origin, payload.dest );
  
  // some limits to catch when things get out of control in a loop
  CORETECH_ASSERT(_segmentPrimitives.size() < 128);
  CORETECH_ASSERT(_segmentPrimitives[payload.identifier].size() < 1024);
}

void PhysVizController::ProcessVizQuadMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_Quad();
  PRINT("Processing DrawQuad (%f %f %f), (%f %f %f), (%f %f %f), (%f %f %f)\n",
    payload.xUpperLeft,  payload.yUpperLeft,  payload.zUpperLeft,
    payload.xLowerLeft,  payload.yLowerLeft,  payload.zLowerLeft,
    payload.xUpperRight, payload.yUpperRight, payload.zUpperRight,
    payload.xLowerRight, payload.yLowerRight, payload.zLowerRight);

  _quadMap[(int)payload.quadType][payload.quadID] = VizInterface::Quad(payload);
}

// OLD SCHEME for memory map quad display

void PhysVizController::ProcessVizSimpleQuadVectorMessageBegin(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_SimpleQuadVectorMessageBegin();
  PRINT("Processing SimpleQuadVectorMessageBegin '%s'\n", payload.identifier.c_str());

  // clear the vector in Incoming
  _simpleQuadVectorMapIncoming[payload.identifier].clear();
}

void PhysVizController::ProcessVizSimpleQuadVectorMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  // WARNING:  No check here that we've received the 'begin' message yet (does not handle out-of-order messages)
  const auto& payload = msg.GetData().Get_SimpleQuadVectorMessage();
  PRINT("Processing SimpleQuadVectorMessage '%s' (quad count: %zu)\n",
    payload.identifier.c_str(), payload.quads.size());

  // ideally I would like to move vectors, but messages are const in case more listeners subscribe
  SimpleQuadVector& dest = _simpleQuadVectorMapIncoming[payload.identifier];
  //  dest.insert(
  //      dest.end(),
  //      std::make_move_iterator(payload.quads.begin()),
  //      std::make_move_iterator(payload.quads.end())
  //    );
  const size_t quadLimitPerId = 8192;
  const size_t newSize = dest.size() + payload.quads.size();
  if( newSize <= quadLimitPerId ) {
    dest.reserve( newSize );
    dest.insert( dest.end(), payload.quads.begin(), payload.quads.end());
  }
  else
  {
    static int warningTimesLeft = 30;
    if ( warningTimesLeft > 0 )
    {
      const size_t available = quadLimitPerId - dest.size();
      const size_t asked     = payload.quads.size();
      const size_t discarded = asked - available;
      --warningTimesLeft;
      dWebotsConsolePrintf("[PhysVizController] [WARNING] Too many quads for '%s'. Discarding %zu from %zu received in this message. (%d warnings left)\n",
        payload.identifier.c_str(),
        discarded,
        asked,
        warningTimesLeft );
      if ( warningTimesLeft == 0 ) {
        dWebotsConsolePrintf("[PhysVizController] [WARNING] Suppressing future warnings from quad excess\n");
      }
    }
    dest.reserve( quadLimitPerId );
    std::vector<VizInterface::SimpleQuad>::const_iterator payloadIt = payload.quads.begin();
    while( (dest.size() < quadLimitPerId) && payloadIt != payload.quads.end() ) {
      dest.emplace_back( *payloadIt );
      ++payloadIt;
    }
  }
}

void PhysVizController::ProcessVizSimpleQuadVectorMessageEnd(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_SimpleQuadVectorMessageEnd();
  PRINT("Processing SimpleQuadVectorMessageEnd '%s'\n", payload.identifier.c_str());

  // swap the vectors from incoming - ready
  std::swap(_simpleQuadVectorMapIncoming[payload.identifier], _simpleQuadVectorMapReady[payload.identifier]);
}


// NEW SCHEME for memory map quad display
  
void PhysVizController::ProcessVizMemoryMapMessageBegin(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_MemoryMapMessageVizBegin();
  _memoryMapInfo[payload.originId] = payload.info;

  // clear the vector in Incoming
  _memoryMapQuadInfoVectorMapIncoming[payload.originId].clear();
}

void PhysVizController::ProcessVizMemoryMapMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_MemoryMapMessageViz();

  MemoryMapQuadInfoVector& dest = _memoryMapQuadInfoVectorMapIncoming[payload.originId];
  dest[payload.seqNum] = std::move(payload.quadInfos);
}

void PhysVizController::ProcessVizMemoryMapMessageEnd(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_MemoryMapMessageVizEnd();

  // Now that we've received the entire list of quad infos, decode them into a list of drawable quads
  
  // input  is _memoryMapQuadInfoDebugVizVectorMapIncoming[payload.originId], and _memoryMapInfo[payload.originId]
  // output is _simpleQuadVectorMapReady["string"]
  const auto& iter = _memoryMapInfo.find(payload.originId);
  if(iter == _memoryMapInfo.end())
  {
    dWebotsConsolePrintf("[PhysVizController] [INFO] Didn't get begin memory map begin viz message");
  }
  else
  {
    const ExternalInterface::MemoryMapInfo& info = iter->second;
    
    SimpleQuadVector& destSimpleQuads = _simpleQuadVectorMapReady[info.identifier];
    destSimpleQuads.clear();
    
    Point3f rootCenter(MM_TO_M(info.rootCenterX), MM_TO_M(info.rootCenterY), MM_TO_M(info.rootCenterZ));
    MemoryMapNode rootNode(info.rootDepth, MM_TO_M(info.rootSize_mm), rootCenter);

    u32 expectedSeqNum = 0;
    const auto& srcQuadInfos = _memoryMapQuadInfoVectorMapIncoming[payload.originId];
    for (const auto& quadInfo : srcQuadInfos)
    {
      if(quadInfo.first != expectedSeqNum)
      {
        dWebotsConsolePrintf("[PhysVizController] [INFO] Dropped memory map viz message, displayed map will be incorrect");
        break;
      }
      else
      {
        for (const auto& quad : quadInfo.second)
        {
          rootNode.AddChild(destSimpleQuads, quad);
        }
        ++expectedSeqNum;
      }
    }
  }
  
  _memoryMapQuadInfoVectorMapIncoming.erase(payload.originId);
  _memoryMapInfo.erase(payload.originId);
}

void PhysVizController::ProcessVizEraseObjectMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_EraseObject();
  PRINT("Processing EraseObject %d\n", payload.objectID);

  if (payload.objectID == (uint32_t)VizConstants::ALL_OBJECT_IDs) {
    _objectMap.clear();
  } else if (payload.objectID == (uint32_t)VizConstants::OBJECT_ID_RANGE) {
    // Get lower bound iterator
    auto lowerIt = _objectMap.lower_bound(payload.lower_bound_id);
    if (lowerIt == _objectMap.end())
      return;

    // Get upper bound iterator
    auto upperIt = _objectMap.upper_bound(payload.upper_bound_id);

    // Erase objects in bounds
    _objectMap.erase(lowerIt, upperIt);

  } else {
    _objectMap.erase(payload.objectID);
  }
}

void PhysVizController::ProcessVizEraseSegmentPrimitivesMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_EraseSegmentPrimitives();
  PRINT("Processing EraseSegmentPrimitives (%s)\n", payload.identifier.c_str());
  
  _segmentPrimitives.erase(payload.identifier);
}

void PhysVizController::ProcessVizEraseQuadMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_EraseQuad();
  PRINT("Processing EraseQuad\n");

  if(payload.quadType == (uint32_t)VizConstants::ALL_QUAD_TYPEs) {
    // NOTE: ignores quad ID
    _quadMap.clear();
  } else {
    auto quadsByType = _quadMap.find(payload.quadType);
    if(quadsByType != _quadMap.end()) {
      if (payload.quadID == (uint32_t)VizConstants::ALL_QUAD_IDs) {
        quadsByType->second.clear();
      } else {
        quadsByType->second.erase(payload.quadID);
      }
    }
  }
}


void PhysVizController::ProcessVizAppendPathSegmentLineMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_AppendPathSegmentLine();
  PRINT("Processing AppendLine\n");

  PathPoint startPt(payload.x_start_m, payload.y_start_m, payload.z_start_m, true);
  PathPoint endPt(payload.x_end_m, payload.y_end_m, payload.z_end_m);

  _pathMap[payload.pathID].push_back(startPt);
  _pathMap[payload.pathID].push_back(endPt);
}

void PhysVizController::ProcessVizAppendPathSegmentArcMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_AppendPathSegmentArc();
  PRINT("Processing AppendArc\n");

  float center_x = payload.x_center_m;
  float center_y = payload.y_center_m;
  float center_z = 0;

  float radius = payload.radius_m;
  float startRad = payload.start_rad;
  float sweepRad = payload.sweep_rad;
  float endRad = startRad + sweepRad;

  float dir = (sweepRad > 0 ? 1 : -1);

  // Make endRad be between -PI and PI
  while (endRad > M_PI_F) {
    endRad -= 2*M_PI_F;
  }
  while (endRad < -M_PI_F) {
    endRad += 2*M_PI_F;
  }


  if (dir == 1) {
    // Make startRad < endRad
    while (startRad > endRad) {
      startRad -= 2*M_PI_F;
    }
  } else {
    // Make startRad > endRad
    while (startRad < endRad) {
      startRad += 2*M_PI_F;
    }
  }

  // Add points along arc from startRad to endRad at arcRes_rad resolution
  float currRad = startRad;
  float dx,dy;//,cosCurrRad;
  //dWebotsConsolePrintf("***** ARC rad %f (%f to %f), radius %f\n", currRad, startRad, endRad, radius);

  bool firstPt = true;
  while (currRad*dir < endRad*dir) {
    dx = (float)(cos(currRad) * radius);
    dy = (float)(sin(currRad) * radius);
    PathPoint pt(center_x + dx, center_y + dy, center_z, firstPt);
    firstPt = false;
    _pathMap[payload.pathID].push_back(pt);
    currRad += dir * _arcRes_rad;
  }

}

void PhysVizController::ProcessVizSetPathColorMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_SetPathColor();
  PRINT("Processing SetPathColor\n");

  _pathColorMap[payload.pathID] = payload.colorID;
}

void PhysVizController::ProcessVizErasePathMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_ErasePath();
  PRINT("Processing ErasePath\n");

  if (payload.pathID == (uint32_t)VizConstants::ALL_PATH_IDs) {
    _pathMap.clear();
  } else {
    _pathMap.erase(payload.pathID);
  }
}

void PhysVizController::ProcessVizDefineColorMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_DefineColor();
  PRINT("Processing DefineColor\n");

  _colorMap[payload.colorID] = payload;
}

void PhysVizController::ProcessVizShowObjectsMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_ShowObjects();
  PRINT("Processing ShowObjects (%d)\n", payload.show);

  _drawEnabled = payload.show > 0;
}

void PhysVizController::ProcessVizSetOriginMessage(const AnkiEvent<VizInterface::MessageViz> &msg)
{
  const auto& payload = msg.GetData().Get_SetVizOrigin();
  
  _globalRotation[0] = RAD_TO_DEG(payload.rot_rad); // Note that global rotation angle is in degrees!
  _globalRotation[1] = payload.rot_axis_x;
  _globalRotation[2] = payload.rot_axis_y;
  _globalRotation[3] = payload.rot_axis_z;
  
  _globalTranslation[0] = MM_TO_M(payload.trans_x_mm);
  _globalTranslation[1] = MM_TO_M(payload.trans_y_mm);
  _globalTranslation[2] = MM_TO_M(payload.trans_z_mm);
  
  PRINT("Processing SetVizOrigin: %.1fdeg @(%.1f %.1f %.1f), (%.1f %.1f %.1f)\n",
        _globalRotation[0], _globalRotation[1], _globalRotation[2], _globalRotation[3],
        _globalTranslation[0], _globalTranslation[1], _globalTranslation[2]);
}
  
void PhysVizController::DrawTextAtOffset(std::string s, float x_off, float y_off, float z_off)
{
  glPushMatrix();
  glTranslatef(x_off, y_off, z_off);
  glRasterPos2i(0,0);
  void * font = GLUT_BITMAP_9_BY_15;
  for (std::string::iterator i = s.begin(); i != s.end(); ++i)
  {
    char c = *i;
    glutBitmapCharacter(font, c);
  }
  glPopMatrix();
}


void PhysVizController::DrawCuboid(float x_dim, float y_dim, float z_dim)
{

  // Webots hack
  float halfX = x_dim * 0.5f;
  float halfY = y_dim * 0.5f;
  float halfZ = z_dim * 0.5f;

  // TOP
  glBegin(GL_LINE_LOOP);
  glVertex3f(  halfX,  halfY,  halfZ );
  glVertex3f(  halfX, -halfY,  halfZ );
  glVertex3f( -halfX, -halfY,  halfZ );
  glVertex3f( -halfX,  halfY,  halfZ );
  glEnd();

  // BOTTOM
  glBegin(GL_LINE_LOOP);
  glVertex3f(  halfX,  halfY, -halfZ );
  glVertex3f(  halfX, -halfY, -halfZ );
  glVertex3f( -halfX, -halfY, -halfZ );
  glVertex3f( -halfX,  halfY, -halfZ );
  glEnd();


  // VERTICAL EDGES
  glBegin(GL_LINES);

  glVertex3f(  halfX,  halfY,  halfZ );
  glVertex3f(  halfX,  halfY,  -halfZ );

  glVertex3f(  halfX, -halfY,  halfZ );
  glVertex3f(  halfX, -halfY,  -halfZ );

  glVertex3f( -halfX,  halfY,  halfZ );
  glVertex3f( -halfX,  halfY,  -halfZ );

  glVertex3f( -halfX, -halfY,  halfZ );
  glVertex3f( -halfX, -halfY,  -halfZ );

  glEnd();
}


void PhysVizController::DrawRamp(float platformLength, float slopeLength, float width, float height)
{
  float halfY = width*0.5f;

  // TOP
  glBegin(GL_LINE_LOOP);
  glVertex3f(  platformLength + slopeLength,  halfY,  height );
  glVertex3f(  platformLength + slopeLength, -halfY,  height );
  glVertex3f(  slopeLength, -halfY,  height );
  glVertex3f(  slopeLength,  halfY,  height );
  glEnd();

  // BOTTOM
  glBegin(GL_LINE_LOOP);
  glVertex3f(  platformLength + slopeLength,  halfY, 0 );
  glVertex3f(  platformLength + slopeLength, -halfY, 0 );
  glVertex3f( 0, -halfY, 0 );
  glVertex3f( 0,  halfY, 0 );
  glEnd();


  // VERTICAL / SLOPED EDGES
  glBegin(GL_LINES);

  glVertex3f(  platformLength + slopeLength,  halfY,  height );
  glVertex3f(  platformLength + slopeLength,  halfY,  0 );

  glVertex3f(  platformLength + slopeLength, -halfY,  height );
  glVertex3f(  platformLength + slopeLength, -halfY,  0 );

  glVertex3f( slopeLength,  halfY,  height );
  glVertex3f( 0,  halfY,  0 );

  glVertex3f( slopeLength, -halfY,  height );
  glVertex3f( 0, -halfY,  0 );

  glEnd();

}

void PhysVizController::DrawHead(float width, float height, float depth)
{
  const float back_scale = 0.8f;
  const float r_hor_front = width * 0.5f;
  const float r_ver_front = height * 0.5f;

  const int N = 20;

  float x_front_next = r_hor_front;
  float z_front_next = 0.f;

  glBegin(GL_LINES);
  for(int i=0; i<=N; ++i) {
    glVertex3f(x_front_next, 0.f, z_front_next);
    glVertex3f(back_scale*x_front_next, depth, back_scale*z_front_next);

    float x_front_prev = x_front_next;
    float z_front_prev = z_front_next;

    const float angle = (float)(2*M_PI*float(i))/float(N);
    x_front_next = r_hor_front * (float)cos(angle);
    z_front_next = r_ver_front * (float)sin(angle);

    glVertex3f(x_front_prev, 0.f, z_front_prev);
    glVertex3f(x_front_next, 0.f, z_front_next);

    glVertex3f(back_scale*x_front_prev, depth, back_scale*z_front_prev);
    glVertex3f(back_scale*x_front_next, depth, back_scale*z_front_next);

  }
  glEnd();

}

// x,y,z: Position of tetrahedron main tip with respect to its origin
// length_x, length_y, length_z: Dimensions of tetrahedron
void PhysVizController::DrawTetrahedronMarker(const float x, const float y, const float z,
  const float length_x, const float length_y, const float length_z)
{
  // Dimensions of tetrahedon-h shape
  const float l = length_x;
  const float half_w = 0.5f * length_y;
  const float h = length_z;

  glBegin(GL_TRIANGLES);

  // Bottom face
  glVertex3f( x, y, z);
  glVertex3f( x-l, y+half_w, z);
  glVertex3f( x-l, y-half_w, z);

  // Left face
  glVertex3f( x, y, z);
  glVertex3f( x-l, y+half_w, z);
  glVertex3f( x-l, y, z+h);

  // Right face
  glVertex3f( x, y, z);
  glVertex3f( x-l, y, z+h);
  glVertex3f( x-l, y-half_w, z);

  // Back face
  glVertex3f( x-l, y, z+h);
  glVertex3f( x-l, y+half_w, z);
  glVertex3f( x-l, y-half_w, z);

  glEnd();

}


void PhysVizController::DrawRobot(Anki::Vector::VizRobotMarkerType type)
{

  // Location of robot origin project up above the head
  float x = 0;
  float y = 0;
  float z = 0.068f;

  // Dimensions
  float l,w,h;

  switch (type) {
    case VizRobotMarkerType::VIZ_ROBOT_MARKER_SMALL_TRIANGLE:
      l = 0.03f;
      w = 0.02f;
      h = 0.01f;
      break;
    case VizRobotMarkerType::VIZ_ROBOT_MARKER_BIG_TRIANGLE:
      x += 0.03f;  // Move tip of marker to come forward, roughly up to lift position.
      l = 0.062f;
      w = 0.08f;
      h = 0.01f;
      break;
  }

  DrawTetrahedronMarker(x, y, z, l, w, h);
}

void PhysVizController::DrawPredockPose()
{
  // Another tetrahedron-y shape like draw_robot that shows where the robot
  // _would_ be if it were positioned at this pre-dock pose
  DrawRobot(VizRobotMarkerType::VIZ_ROBOT_MARKER_SMALL_TRIANGLE);
}

void PhysVizController::DrawQuad(const float xUpperLeft,  const float yUpperLeft, const float zUpperLeft,
  const float xLowerLeft,  const float yLowerLeft, const float zLowerLeft,
  const float xUpperRight, const float yUpperRight, const float zUpperRight,
  const float xLowerRight, const float yLowerRight, const float zLowerRight)
{
  glBegin(GL_LINE_LOOP);
  glVertex3f(xUpperLeft,  yUpperLeft,  zUpperLeft );
  glVertex3f(xUpperRight, yUpperRight, zUpperRight);
  glVertex3f(xLowerRight, yLowerRight, zLowerRight);
  glVertex3f(xLowerLeft,  yLowerLeft,  zLowerLeft );
  glEnd();
}

void PhysVizController::DrawQuadFill(
  const float xUpperLeft,  const float yUpperLeft, const float zUpperLeft,
  const float xLowerLeft,  const float yLowerLeft, const float zLowerLeft,
  const float xUpperRight, const float yUpperRight, const float zUpperRight,
  const float xLowerRight, const float yLowerRight, const float zLowerRight)
{
  glBegin(GL_TRIANGLE_FAN);
  glVertex3f(xUpperLeft,  yUpperLeft,  zUpperLeft );
  glVertex3f(xLowerLeft,  yLowerLeft,  zLowerLeft );
  glVertex3f(xLowerRight, yLowerRight, zLowerRight);
  glVertex3f(xUpperRight, yUpperRight, zUpperRight);
  glEnd();
}


MemoryMapNode::MemoryMapNode(int depth, float size_m, const Point3f& center)
{
  _depth = depth;
  _size_m = size_m;
  _center = center;
  _nextChild = 0;
}

bool MemoryMapNode::AddChild(SimpleQuadVector& destSimpleQuads, const ExternalInterface::MemoryMapQuadInfo& quad)
{
  using namespace ExternalInterface;
  
  if (_depth == quad.depth)
  {
    VizInterface::SimpleQuad simpleQuad;
    simpleQuad.center[0] = _center.x();
    simpleQuad.center[1] = _center.y();
    simpleQuad.center[2] = _center.z();
    simpleQuad.sideSize = _size_m;
    simpleQuad.color = ColorRGBA(quad.colorRGBA);
    destSimpleQuads.emplace_back(simpleQuad);
    return true;
  }
  
  if (_children.empty())
  {
    int nextDepth = _depth - 1;
    float nextSize = _size_m * 0.5f;
    float offset = nextSize * 0.5f;
    Point3f center1(_center.x() + offset, _center.y() + offset, _center.z());
    Point3f center2(_center.x() + offset, _center.y() - offset, _center.z());
    Point3f center3(_center.x() - offset, _center.y() + offset, _center.z());
    Point3f center4(_center.x() - offset, _center.y() - offset, _center.z());
    // this code should probably be optimized to avoid copies but I'm just getting it to work right now
    
    _children.push_back(MemoryMapNode(nextDepth, nextSize, center1));
    _children.push_back(MemoryMapNode(nextDepth, nextSize, center2));
    _children.push_back(MemoryMapNode(nextDepth, nextSize, center3));
    _children.push_back(MemoryMapNode(nextDepth, nextSize, center4));
  }
  
  if (_children[_nextChild].AddChild(destSimpleQuads, quad))
  {
    // All children below this child have been processed
    _nextChild++;
  }
  
  return (_nextChild > 3);
}

} // end namespace Vector
} // end namespace Anki
