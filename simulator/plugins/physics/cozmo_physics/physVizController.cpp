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
#include "anki/common/constantsAndMacros.h"
#include "anki/cozmo/basestation/blockWorld.h"
#include "anki/cozmo/basestation/namedColors/namedColors.h"
#include "anki/cozmo/basestation/viz/vizObjectBaseId.h"
#include "anki/common/basestation/colorRGBA.h"
#include <OpenGL/OpenGL.h>
#include <GLUT/GLUT.h>

#if DEBUG_COZMO_PHYSICS
#define PRINT(...) dWebotsConsolePrintf(__VA_ARGS__)
#else
#define PRINT(...)
#endif

namespace Anki {
namespace Cozmo {


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

  _server.StartListening((uint16_t)VizConstants::PHYSICS_PLUGIN_SERVER_PORT);

  _pathMap.clear();

}

void PhysVizController::Update()
{
  const size_t maxPacketSize{(size_t)VizConstants::MaxMessageSize};
  uint8_t data[maxPacketSize]{0};
  int numBytesRecvd;
  ///// Process messages from basestation /////
  while ((numBytesRecvd = _server.Recv((char*)data, maxPacketSize)) > 0) {
    ProcessMessage(VizInterface::MessageViz(data, (size_t)numBytesRecvd));
  }

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
    glColor4ub(NamedColors::DEFAULT.r(),
      NamedColors::DEFAULT.g(),
      NamedColors::DEFAULT.b(),
      NamedColors::DEFAULT.alpha());

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
          Anki::Cozmo::VizDefineColor *c = &(cIt->second);
          glColor3f(c->r, c->g, c->b);

        }
         */
      }

      // Draw the path
      glBegin(GL_LINE_STRIP);

      for (auto pathIt = pathMapIt->second.begin(); pathIt != pathMapIt->second.end(); pathIt++) {
        glVertex3f((*pathIt)[0],(*pathIt)[1],(*pathIt)[2] + _heightOffset);
      }
      glEnd();

      // Restore default color
      //glColor3f(DEFAULT_COLOR[0], DEFAULT_COLOR[1], DEFAULT_COLOR[2]);
      glColor4ub(NamedColors::DEFAULT.r(),
        NamedColors::DEFAULT.g(),
        NamedColors::DEFAULT.b(),
        NamedColors::DEFAULT.alpha());
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
        Anki::Cozmo::VizDefineColor *c = &(cIt->second);
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
          std::string idString = std::to_string(obj->objectID - Anki::Cozmo::VizObjectBaseID[(int)VizObjectType::VIZ_OBJECT_CUBOID]);
          DrawTextAtOffset(idString, 0.6*obj->x_size_m, 0.6*obj->y_size_m, 0.6*obj->z_size_m);
          DrawTextAtOffset(idString, -0.6*obj->x_size_m, -0.6*obj->y_size_m, -0.6*obj->z_size_m);

          // AXES:
          glColor4ub(255, 0, 0, 255);
          glBegin(GL_LINES);
          glVertex3f(0,0,0);
          glVertex3f(obj->x_size_m, 0, 0);
          glEnd();

          glColor4ub(0, 255, 0, 255);
          glBegin(GL_LINES);
          glVertex3f(0, 0, 0);
          glVertex3f(0, obj->y_size_m, 0);
          glEnd();

          glColor4ub(0, 0, 255, 255);
          glBegin(GL_LINES);
          glVertex3f(0, 0, 0);
          glVertex3f(0, 0, obj->z_size_m);
          glEnd();

          break;
        }
        case VizObjectType::VIZ_OBJECT_RAMP:
        {
          float slopeLength = obj->params[0]*obj->x_size_m;
          DrawRamp(obj->x_size_m, slopeLength, obj->y_size_m, obj->z_size_m);
          break;
        }
        case VizObjectType::VIZ_OBJECT_CHARGER:
        {
          float slopeLength = obj->params[0]*obj->x_size_m;
          DrawRamp(obj->x_size_m, slopeLength, obj->y_size_m, obj->z_size_m);
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

      glFlush();

      glPopMatrix();

      // Restore default color
      //glColor3f(DEFAULT_COLOR[0], DEFAULT_COLOR[1], DEFAULT_COLOR[2]);
      glColor4ub(NamedColors::DEFAULT.r(),
        NamedColors::DEFAULT.g(),
        NamedColors::DEFAULT.b(),
        NamedColors::DEFAULT.alpha());

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
          Anki::Cozmo::VizDefineColor *c = &(cIt->second);
          glColor3f(c->r, c->g, c->b);
        }
         */

        DrawQuad(quad.xUpperLeft,  quad.yUpperLeft,  quad.zUpperLeft,
          quad.xLowerLeft,  quad.yLowerLeft,  quad.zLowerLeft,
          quad.xUpperRight, quad.yUpperRight, quad.zUpperRight,
          quad.xLowerRight, quad.yLowerRight, quad.zLowerRight);

        // Restore default color
        //glColor3f(DEFAULT_COLOR[0], DEFAULT_COLOR[1], DEFAULT_COLOR[2]);
        glColor4ub(NamedColors::DEFAULT.r(),
          NamedColors::DEFAULT.g(),
          NamedColors::DEFAULT.b(),
          NamedColors::DEFAULT.alpha());
      } // for each quad
    } // for each quad type

  }
}


void PhysVizController::Cleanup()
{
  _server.StopListening();
  _pathMap.clear();
  _pathColorMap.clear();
  _objectMap.clear();
  _quadMap.clear();
  _colorMap.clear();
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

void PhysVizController::ProcessVizQuadMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = msg.GetData().Get_Quad();
  PRINT("Processing DrawQuad (%f %f %f), (%f %f %f), (%f %f %f), (%f %f %f)\n",
    payload.xUpperLeft,  payload.yUpperLeft,  payload.zUpperLeft,
    payload.xLowerLeft,  payload.yLowerLeft,  payload.zLowerLeft,
    payload.xUpperRight, payload.yUpperRight, payload.zUpperRight,
    payload.xLowerRight, payload.yLowerRight, payload.zLowerRight);

  _quadMap[payload.quadType][payload.quadID] = VizInterface::Quad(payload);
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

void PhysVizController::ProcessVizEraseQuadMessage(const AnkiEvent<VizInterface::MessageViz>& msg)
{
  const auto& payload = payload.GetData().Get_EraseQuad();
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
  const auto& payload = payload.GetData().Get_AppendPathSegmentLine();
  PRINT("Processing AppendLine\n");

  std::vector<float> startPt = {payload.x_start_m, payload.y_start_m, payload.z_start_m};
  std::vector<float> endPt = {payload.x_end_m, payload.y_end_m, payload.z_end_m};

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
  while (endRad > PI) {
    endRad -= 2*PI;
  }
  while (endRad < -PI) {
    endRad += 2*PI;
  }


  if (dir == 1) {
    // Make startRad < endRad
    while (startRad > endRad) {
      startRad -= 2*PI;
    }
  } else {
    // Make startRad > endRad
    while (startRad < endRad) {
      startRad += 2*PI;
    }
  }

  // Add points along arc from startRad to endRad at arcRes_rad resolution
  float currRad = startRad;
  float dx,dy;//,cosCurrRad;
  //dWebotsConsolePrintf("***** ARC rad %f (%f to %f), radius %f\n", currRad, startRad, endRad, radius);

  while (currRad*dir < endRad*dir) {
    dx = (float)(cos(currRad) * radius);
    dy = (float)(sin(currRad) * radius);
    std::vector<float> pt = {center_x + dx, center_y + dy, center_z};
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


void PhysVizController::DrawRobot(Anki::Cozmo::VizRobotMarkerType type)
{

  // Location of robot origin project up above the head
  float x = 0;
  float y = 0;
  float z = 0.068;

  // Dimensions
  float l,w,h;

  switch (type) {
    case VizRobotMarkerType::VIZ_ROBOT_MARKER_SMALL_TRIANGLE:
      l = 0.03;
      w = 0.02;
      h = 0.01;
      break;
    case VizRobotMarkerType::VIZ_ROBOT_MARKER_BIG_TRIANGLE:
      x += 0.03;  // Move tip of marker to come forward, roughly up to lift position.
      l = 0.062;
      w = 0.08;
      h = 0.01;
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

} // end namespace Cozmo
} // end namespace Anki
