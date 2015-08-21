/*
 * File:  cozmo_physics.cpp        
 * Date:  09-25-2013        
 * Description: Webots physics plugin for drawing paths in the cozmo simulator. 
 *              Paths currently consist only of arcs and straights.
 *              This plugin receives messages from the sim_viz methods
 *              via an Emitter. See VizMsgDefs.h for message formats.
 *  
 * Author: Kevin Yoon       
 * Modifications: 
 */

#include <ode/ode.h>
#include <plugins/physics.h>

#include "cozmo_physics.h"
#include <map>
#include <vector>

#include "anki/cozmo/shared/VizStructs.h"
#include "anki/messaging/shared/UdpServer.h"

#include "anki/common/basestation/colorRGBA.h"


#define DEBUG_COZMO_PHYSICS 0


// Types for paths
typedef std::vector<float> PathVertex_t;
typedef std::vector<PathVertex_t> Path_t;
typedef std::map<u32, Path_t > PathMap_t;

// Map of all paths indexed by robotID and pathID
static PathMap_t pathMap_;

// Map of pathID to colorID
static std::map<u32, u32> pathColorMap_;

// Static objects
typedef std::map<u32, Anki::Cozmo::VizObject> VizObject_t;
static VizObject_t objectMap_;

// Static quads
typedef std::map<u32, Anki::Cozmo::VizQuad> VizQuadMap_t;
typedef std::map<u32, VizQuadMap_t> VizQuadTypeMap_t;
static VizQuadTypeMap_t quadMap_;

/*
// Mat Markers
typedef std::map<u32, Anki::Cozmo::VizQuad> VizMatMarkerMap_t;
static VizMatMarkerMap_t matMarkerMap_;

// Planner Obstacles
typedef std::map<u32, Anki::Cozmo::VizQuad> VizPlannerObstacleMap_t;
static VizPlannerObstacleMap_t plannerObstacleMap_;
*/

// Color map
typedef std::map<u32, Anki::Cozmo::VizDefineColor> VizColorDef_t;
static VizColorDef_t colorMap_;

// Default color for all objects and paths
//static const f32 DEFAULT_COLOR[3] = {1.0, 0.8, 0.0};

// Server that listens for visualization messages from basestation's VizManger
static UdpServer server;

// Whether or not to draw anything
static bool drawEnabled_ = true;

// Default height offset of paths (m)
static float heightOffset_ = 0.045;

// Default angular resolution of arc path segments (radians)
static float arcRes_rad = 0.2;

//const int SIZEOF_FLOAT = sizeof(float);

#if DEBUG_COZMO_PHYSICS
#define PRINT(...) dWebotsConsolePrintf(__VA_ARGS__)
#else
#define PRINT(...)
#endif


namespace Anki {
  namespace Cozmo{

#define MESSAGE_DEFINITION_MODE MESSAGE_DISPATCH_DEFINITION_MODE
#include "anki/cozmo/shared/VizMsgDefs.h"
    
    typedef void (*DispatchFcn_t)(const u8* buffer);
    
    const size_t NUM_TABLE_ENTRIES = Anki::Cozmo::NUM_VIZ_MSG_IDS + 1;
    DispatchFcn_t DispatchTable_[NUM_TABLE_ENTRIES] = {
      0, // Empty entry for NO_MESSAGE_ID
#undef  MESSAGE_DEFINITION_MODE
#define MESSAGE_DEFINITION_MODE MESSAGE_DISPATCH_FCN_TABLE_DEFINITION_MODE
#include "anki/cozmo/shared/VizMsgDefs.h"
      0 // Final dummy entry without comma at end
    };

    
    void ProcessVizObjectMessage(const VizObject& msg)
    {
      PRINT("Processing DrawObject %d %d (%f %f %f) (%f %f %f %f) %d \n",
            msg.objectID,
            msg.objectTypeID,
            msg.x_trans_m, msg.y_trans_m, msg.z_trans_m,
            msg.rot_deg, msg.rot_axis_x, msg.rot_axis_y, msg.rot_axis_z,
            msg.color);
      
      objectMap_[msg.objectID] = msg;
    }
    
    void ProcessVizQuadMessage(const VizQuad& msg)
    {
      PRINT("Processing DrawQuad (%f %f %f), (%f %f %f), (%f %f %f), (%f %f %f)\n",
            msg.xUpperLeft,  msg.yUpperLeft,  msg.zUpperLeft,
            msg.xLowerLeft,  msg.yLowerLeft,  msg.zLowerLeft,
            msg.xUpperRight, msg.yUpperRight, msg.zUpperRight,
            msg.xLowerRight, msg.yLowerRight, msg.zLowerRight);
      
      quadMap_[msg.quadType][msg.quadID] = msg;
    }
    
    void ProcessVizEraseObjectMessage(const VizEraseObject& msg)
    {
      PRINT("Processing EraseObject\n");
      
      if (msg.objectID == ALL_OBJECT_IDs) {
        objectMap_.clear();
      } else if (msg.objectID == OBJECT_ID_RANGE) {
        // Get lower bound iterator
        VizObject_t::iterator lowerIt = objectMap_.lower_bound(msg.lower_bound_id);
        if (lowerIt == objectMap_.end())
          return;
        
        // Get upper bound iterator
        VizObject_t::iterator upperIt = objectMap_.upper_bound(msg.upper_bound_id);
        
        // Erase objects in bounds
        objectMap_.erase(lowerIt, upperIt);
        
      } else {
        objectMap_.erase(msg.objectID);
      }
    }
    
    void ProcessVizEraseQuadMessage(const VizEraseQuad& msg)
    {
      PRINT("Processing EraseQuad\n");
      
      if(msg.quadType == ALL_QUAD_TYPEs) {
        // NOTE: ignores quad ID
        quadMap_.clear();
      } else {
        auto quadsByType = quadMap_.find(msg.quadType);
        if(quadsByType != quadMap_.end()) {
          if (msg.quadID == ALL_QUAD_IDs) {
            quadsByType->second.clear();
          } else {
            quadsByType->second.erase(msg.quadID);
          }
        }
      }
    }
    
    
    void ProcessVizAppendPathSegmentLineMessage(const VizAppendPathSegmentLine &msg)
    {
      PRINT("Processing AppendLine\n");
      
      PathVertex_t startPt = {msg.x_start_m, msg.y_start_m, msg.z_start_m};
      PathVertex_t endPt = {msg.x_end_m, msg.y_end_m, msg.z_end_m};
      
      pathMap_[msg.pathID].push_back(startPt);
      pathMap_[msg.pathID].push_back(endPt);
    }
    
    void ProcessVizAppendPathSegmentArcMessage(const VizAppendPathSegmentArc &msg)
    {
      PRINT("Processing AppendArc\n");
      
      float center_x = msg.x_center_m;
      float center_y = msg.y_center_m;
      float center_z = 0;
      
      float radius = msg.radius_m;
      float startRad = msg.start_rad;
      float sweepRad = msg.sweep_rad;
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
        dx = cos(currRad) * radius;
        dy = sin(currRad) * radius;
        PathVertex_t pt = {center_x + dx, center_y + dy, center_z};
        pathMap_[msg.pathID].push_back(pt);
        currRad += dir * arcRes_rad;
      }
      
    }
    
    void ProcessVizSetPathColorMessage(const VizSetPathColor& msg)
    {
      PRINT("Processing SetPathColor\n");
      
      pathColorMap_[msg.pathID] = msg.colorID;
    }
    
    void ProcessVizErasePathMessage(const VizErasePath& msg)
    {
      PRINT("Processing ErasePath\n");
      
      if (msg.pathID == ALL_PATH_IDs) {
        pathMap_.clear();
      } else {
        pathMap_.erase(msg.pathID);
      }
    }
    
    void ProcessVizDefineColorMessage(const VizDefineColor& msg)
    {
      PRINT("Processing DefineColor\n");
      
      colorMap_[msg.colorID] = msg;
    }
    
    void ProcessVizShowObjectsMessage(const VizShowObjects& msg)
    {
      PRINT("Processing ShowObjects (%d)\n", msg.show);
      
      drawEnabled_ = msg.show > 0;
    }
    
    
    // Stubs
    // These messages are handled by cozmo_viz_controller
    void ProcessVizSetLabelMessage(const VizSetLabel& msg){};
    void ProcessVizDockingErrorSignalMessage(const VizDockingErrorSignal& msg){};
    void ProcessVizImageChunkMessage(const VizImageChunk& msg){};
    void ProcessVizSetRobotMessage(const VizSetRobot& msg){};
    void ProcessVizTrackerQuadMessage(const VizTrackerQuad& msg){};
    void ProcessVizVisionMarkerMessage(const VizVisionMarker& msg){};
    void ProcessVizCameraQuadMessage(const VizCameraQuad& msg) {};
    void ProcessVizCameraLineMessage(const VizCameraLine& msg) {};
    void ProcessVizCameraOvalMessage(const VizCameraOval& msg) {};
    void ProcessVizCameraTextMessage(const VizCameraText& msg) {};
    void ProcessVizRobotStateMessage(const VizRobotState& msg) {};
    
  } // namespace Cozmo
} // namespace Anki


/*
 * Note: This plugin will become operational only after it was compiled and associated with the current world (.wbt).
 * To associate this plugin with the world follow these steps:
 *  1. In the Scene Tree, expand the "WorldInfo" node and select its "physics" field
 *  2. Then hit the [Select] button at the bottom of the Scene Tree
 *  3. In the list choose the name of this plugin (same as this file without the extention)
 *  4. Then save the .wbt by hitting the "Save" button in the toolbar of the 3D view
 *  5. Then revert the simulation: the plugin should now load and execute with the current simulation
 */

void webots_physics_init() {
  /*
   * Get ODE object from the .wbt model, e.g.
   *   dBodyID body1 = dWebotsGetBodyFromDEF("MY_ROBOT");
   *   dBodyID body2 = dWebotsGetBodyFromDEF("MY_SERVO");
   *   dGeomID geom2 = dWebotsGetGeomFromDEF("MY_SERVO");
   * If an object is not found in the .wbt world, the function returns NULL.
   * Your code should correcly handle the NULL cases because otherwise a segmentation fault will crash Webots.
   *
   * This function is also often used to add joints to the simulation, e.g.
   *   dJointID joint = dJointCreateBall(world, 0);
   *   dJointAttach(joint, body1, body2);
   *   ...
   */
  
  server.StartListening(Anki::Cozmo::PHYSICS_PLUGIN_SERVER_PORT);
  
  pathMap_.clear();
}

void webots_physics_step() {
  /*
   * Do here what needs to be done at every time step, e.g. add forces to bodies
   *   dBodyAddForce(body1, f[0], f[1], f[2]);
   *   ...
   */

  int bytes_recvd = 0;
  const int MAX_RECV_BUF_SIZE = 1024;
  u8 recvBuf[MAX_RECV_BUF_SIZE];
  
  
  /*
  //// Process draw messages from CozmoBot ////
  // Since data isn't packetized, need to process multiple packets per read.
  u8 *tmpBuf = (u8*)dWebotsReceive(&bytes_recvd);
  static int recvBufSize = 0;
  if (bytes_recvd > 0) {
    memcpy(recvBuf + recvBufSize, tmpBuf, bytes_recvd);
    recvBufSize += bytes_recvd;

    // Process every whole message
    int curr_byte = 0;
    while (curr_byte < recvBufSize) {

      int msgID = static_cast<Anki::Cozmo::VizMsgID>(recvBuf[curr_byte]);
      int msgSize =  Anki::Cozmo::LookupTable_[msgID].size;
      curr_byte += 1; // Skip the msgID byte
      
      PRINT( "Processing msgs from Webots: Got msg %d (%d bytes expected, %d bytes left)\n", msgID, msgSize, recvBufSize - curr_byte);
      
      if (recvBufSize - curr_byte >= msgSize) {
        // There is a complete message in recvBuf. Process it.
        (*Anki::Cozmo::LookupTable_[msgID].dispatchFcn)(recvBuf + curr_byte);
        curr_byte += msgSize;
      } else {
        // There are not enough bytes in the buffer for a complete message.
        // Move remaining bytes to front of buffer.
        memmove(recvBuf, recvBuf + curr_byte, recvBufSize - curr_byte);
        break;
      }
    }
    recvBufSize -= curr_byte;
  }
   */
  
  
  ///// Process messages from basestation /////
  while ((bytes_recvd = server.Recv((char*)recvBuf, MAX_RECV_BUF_SIZE)) > 0) {
    int msgID = static_cast<Anki::Cozmo::VizMsgID>(recvBuf[0]);
    PRINT( "Processing msgs from Basestation: Got msg %d (%d bytes)\n", msgID, bytes_recvd);
    
    (*Anki::Cozmo::DispatchTable_[msgID])(recvBuf + 1);
  }

}


void draw_cuboid(float x_dim, float y_dim, float z_dim)
{
  
   // Webots hack
  float halfX = x_dim*0.5;
  float halfY = y_dim*0.5;
  float halfZ = z_dim*0.5;
  
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

  
  glFlush();
}


void draw_ramp(float platformLength, float slopeLength, float width, float height)
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
  
  
  glFlush();

}

void draw_head(float width, float height, float depth)
{
  const float back_scale = 0.8f;
  const float r_hor_front = width*0.5f;
  const float r_ver_front = height*0.5f;
  
  const int N = 20;

  float x_front_next = r_hor_front;
  float z_front_next = 0.f;

  glBegin(GL_LINES);
  for(int i=0; i<=N; ++i) {
    glVertex3f(x_front_next, 0.f, z_front_next);
    glVertex3f(back_scale*x_front_next, depth, back_scale*z_front_next);
    
    float x_front_prev = x_front_next;
    float z_front_prev = z_front_next;
    
    const float angle = (2*M_PI*float(i))/float(N);
    x_front_next = r_hor_front*cos(angle);
    z_front_next = r_ver_front*sin(angle);
  
    glVertex3f(x_front_prev, 0.f, z_front_prev);
    glVertex3f(x_front_next, 0.f, z_front_next);
    
    glVertex3f(back_scale*x_front_prev, depth, back_scale*z_front_prev);
    glVertex3f(back_scale*x_front_next, depth, back_scale*z_front_next);
    
  }
  glEnd();
  
}

// x,y,z: Position of tetrahedron main tip with respect to its origin
// length_x, length_y, length_z: Dimensions of tetrahedron
void draw_tetrahedron_marker(const float x, const float y, const float z,
                             const float length_x, const float length_y, const float length_z)
{
  // Dimensions of tetrahedon-h shape
  const float l = length_x;
  const float half_w = 0.5*length_y;
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


void draw_robot(Anki::Cozmo::VizRobotMarkerType type)
{
  
  // Location of robot origin project up above the head
  float x = 0;
  float y = 0;
  float z = 0.068;

  // Dimensions
  float l,w,h;
  
  switch (type) {
    case Anki::Cozmo::VIZ_ROBOT_MARKER_SMALL_TRIANGLE:
      l = 0.03;
      w = 0.02;
      h = 0.01;
      break;
    case Anki::Cozmo::VIZ_ROBOT_MARKER_BIG_TRIANGLE:
      x += 0.03;  // Move tip of marker to come forward, roughly up to lift position.
      l = 0.062;
      w = 0.08;
      h = 0.01;
      break;
    default:
      break;
  }
  
  draw_tetrahedron_marker(x, y, z, l, w, h);
}

void draw_predockpose()
{
  // Another tetrahedron-y shape like draw_robot that shows where the robot
  // _would_ be if it were positioned at this pre-dock pose
  
  draw_robot(Anki::Cozmo::VIZ_ROBOT_MARKER_SMALL_TRIANGLE);
}

void draw_quad(const f32 xUpperLeft,  const f32 yUpperLeft, const f32 zUpperLeft,
               const f32 xLowerLeft,  const f32 yLowerLeft, const f32 zLowerLeft,
               const f32 xUpperRight, const f32 yUpperRight, const f32 zUpperRight,
               const f32 xLowerRight, const f32 yLowerRight, const f32 zLowerRight)
{
  glBegin(GL_LINE_LOOP);
  glVertex3f(xUpperLeft,  yUpperLeft,  zUpperLeft );
  glVertex3f(xUpperRight, yUpperRight, zUpperRight);
  glVertex3f(xLowerRight, yLowerRight, zLowerRight);
  glVertex3f(xLowerLeft,  yLowerLeft,  zLowerLeft );
  glEnd();
}

void webots_physics_draw(int pass, const char *view) {
  
  if (!drawEnabled_) return;
 
  // Only draw in main 3D view (view == NULL) and not the camera views
  if (pass == 1 && view == NULL) {
    
    // setup draw style
    glDisable(GL_LIGHTING);
    glLineWidth(2);

    // Set default color
    //glColor3f(DEFAULT_COLOR[0], DEFAULT_COLOR[1], DEFAULT_COLOR[2]);
    glColor4ub(Anki::NamedColors::DEFAULT.r(),
               Anki::NamedColors::DEFAULT.g(),
               Anki::NamedColors::DEFAULT.b(),
               Anki::NamedColors::DEFAULT.alpha());
    
    // Draw path
    PathMap_t::iterator pathMapIt;
    for (pathMapIt = pathMap_.begin(); pathMapIt != pathMap_.end(); pathMapIt++) {

      int pathID = pathMapIt->first;
      
      // Set path color
      
      if (pathColorMap_.find(pathID) != pathColorMap_.end()) {
        Anki::ColorRGBA pathColor(pathColorMap_[pathID]);
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
      
      for (Path_t::iterator pathIt = pathMapIt->second.begin(); pathIt != pathMapIt->second.end(); pathIt++) {
        glVertex3f((*pathIt)[0],(*pathIt)[1],(*pathIt)[2] + heightOffset_);
      }
      glEnd();

      // Restore default color
      //glColor3f(DEFAULT_COLOR[0], DEFAULT_COLOR[1], DEFAULT_COLOR[2]);
      glColor4ub(Anki::NamedColors::DEFAULT.r(),
                 Anki::NamedColors::DEFAULT.g(),
                 Anki::NamedColors::DEFAULT.b(),
                 Anki::NamedColors::DEFAULT.alpha());
    }
    
    
    // Draw objects
    VizObject_t::iterator objIt;
    for (objIt = objectMap_.begin(); objIt != objectMap_.end(); ++objIt) {
     
      Anki::Cozmo::VizObject *obj = &(objIt->second);
      
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
        case Anki::Cozmo::VIZ_OBJECT_ROBOT:
          draw_robot(Anki::Cozmo::VIZ_ROBOT_MARKER_SMALL_TRIANGLE);
          break;
        case Anki::Cozmo::VIZ_OBJECT_CUBOID:
          draw_cuboid(obj->x_size_m, obj->y_size_m, obj->z_size_m);
          
          
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
        case Anki::Cozmo::VIZ_OBJECT_RAMP:
        {
          float slopeLength = obj->params[0]*obj->x_size_m;
          draw_ramp(obj->x_size_m, slopeLength, obj->y_size_m, obj->z_size_m);
          break;
        }
        case Anki::Cozmo::VIZ_OBJECT_CHARGER:
        {
          float slopeLength = obj->params[0]*obj->x_size_m;
          draw_ramp(obj->x_size_m, slopeLength, obj->y_size_m, obj->z_size_m);
          break;
        }
        case Anki::Cozmo::VIZ_OBJECT_PREDOCKPOSE:
          draw_predockpose();
          break;
          
        case Anki::Cozmo::VIZ_OBJECT_HUMAN_HEAD:
          draw_head(obj->x_size_m, obj->y_size_m, obj->z_size_m);
          break;
          
        default:
          PRINT("Unknown objectTypeID %d\n", obj->objectTypeID);
          break;
      }
      
      glPopMatrix();
      
      // Restore default color
      //glColor3f(DEFAULT_COLOR[0], DEFAULT_COLOR[1], DEFAULT_COLOR[2]);
      glColor4ub(Anki::NamedColors::DEFAULT.r(),
                 Anki::NamedColors::DEFAULT.g(),
                 Anki::NamedColors::DEFAULT.b(),
                 Anki::NamedColors::DEFAULT.alpha());
      
    } // for each object
    
    // Draw quads
    for(auto & quadsByType : quadMap_) {
      for(auto & quadsByID : quadsByType.second) {
        
        const Anki::Cozmo::VizQuad& quad = quadsByID.second;
        
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
        
        draw_quad(quad.xUpperLeft,  quad.yUpperLeft,  quad.zUpperLeft,
                  quad.xLowerLeft,  quad.yLowerLeft,  quad.zLowerLeft,
                  quad.xUpperRight, quad.yUpperRight, quad.zUpperRight,
                  quad.xLowerRight, quad.yLowerRight, quad.zLowerRight);
        
        // Restore default color
        //glColor3f(DEFAULT_COLOR[0], DEFAULT_COLOR[1], DEFAULT_COLOR[2]);
        glColor4ub(Anki::NamedColors::DEFAULT.r(),
                   Anki::NamedColors::DEFAULT.g(),
                   Anki::NamedColors::DEFAULT.b(),
                   Anki::NamedColors::DEFAULT.alpha());
      } // for each quad
    } // for each quad type
    
  }
}

int webots_physics_collide(dGeomID g1, dGeomID g2) {
  /*
   * This function needs to be implemented if you want to overide Webots collision detection.
   * It must return 1 if the collision was handled and 0 otherwise. 
   * Note that contact joints should be added to the contactJointGroup, e.g.
   *   n = dCollide(g1, g2, MAX_CONTACTS, &contact[0].geom, sizeof(dContact));
   *   ...
   *   dJointCreateContact(world, contactJointGroup, &contact[i])
   *   dJointAttach(contactJoint, body1, body2);
   *   ...
   */
  return 0;
}

void webots_physics_cleanup() {
  /*
   * Here you need to free any memory you allocated in above, close files, etc.
   * You do not need to free any ODE object, they will be freed by Webots.
   */
  
  server.StopListening();

  pathMap_.clear();
  pathColorMap_.clear();
  objectMap_.clear();
  quadMap_.clear();
  colorMap_.clear();
}






