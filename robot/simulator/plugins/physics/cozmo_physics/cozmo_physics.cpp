/*
 * File:  cozmo_physics.cpp        
 * Date:  09-25-2013        
 * Description: Webots physics plugin for drawing paths in the cozmo simulator. 
 *              Paths currently consist only of arcs and straights.
 *              This plugin receives messages from the CozmoBot class
 *              via an Emitter. For simplicity, a message is just an array of floats.
 *              The size of the array is determined by the message type.
 *              The message format is defined in cozmo_physics.h.
 *  
 * Author: Kevin Yoon       
 * Modifications: 
 */

#include <ode/ode.h>
#include <plugins/physics.h>

#include "cozmo_physics.h"
#include <map>
#include <vector>

#include "anki/cozmo/VizStructs.h"
#include "anki/messaging/shared/UdpServer.h"


#define DEBUG_COZMO_PHYSICS 0


// Types for paths
typedef std::vector<float> PathVertex_t;
typedef std::vector<PathVertex_t> Path_t;
typedef std::map<int, Path_t > PathMap_t;

// Map of all paths indexed by robotID and pathID
static PathMap_t pathMap_;

// Map of pathID to colorID
static std::map<int, int> pathColorMap_;

// Static objects
typedef std::map<int, Anki::Cozmo::VizObject> VizObject_t;
static VizObject_t objectMap_;

// Color map
typedef std::map<int, Anki::Cozmo::VizDefineColor> VizColorDef_t;
static VizColorDef_t colorMap_;

// Default color for all objects and paths
static const f32 DEFAULT_COLOR[3] = {1.0, 0.8, 0.0};

// Server that listens for visualization messages from basestation's VizManger
static UdpServer server;

// Whether or not to draw paths
static bool drawPaths_ = true;

// Default height offset of paths (m)
static float heightOffset_ = 0.1;

// Default angular resolution of arc path segments (radians)
static float arcRes_rad = 0.2;

const int SIZEOF_FLOAT = sizeof(float);

#if DEBUG_COZMO_PHYSICS
#define PRINT(...) dWebotsConsolePrintf(__VA_ARGS__)
#else
#define PRINT(...)
#endif


namespace Anki {
  namespace Cozmo{
    
    // TODO: Move the following to VizStructs.cpp?
#define MESSAGE_DEFINITION_MODE MESSAGE_DISPATCH_DEFINITION_MODE
#include "anki/cozmo/VizMsgDefs.h"
    
    typedef struct {
      u8 priority;
      u8 size;
      void (*dispatchFcn)(const u8* buffer);
    } TableEntry;
    
    const size_t NUM_TABLE_ENTRIES = Anki::Cozmo::NUM_VIZ_MSG_IDS + 1;
    TableEntry LookupTable_[NUM_TABLE_ENTRIES] = {
      {0, 0, 0}, // Empty entry for NO_MESSAGE_ID
#undef  MESSAGE_DEFINITION_MODE
#define MESSAGE_DEFINITION_MODE MESSAGE_TABLE_DEFINITION_MODE
#include "anki/cozmo/VizMsgDefs.h"
      {0, 0, 0} // Final dummy entry without comma at end
    };
    
    void ProcessVizObjectMessage(const VizObject& msg)
    {
      PRINT("Processing SetVizObject %d %d (%f %f %f) (%f %f %f %f) %d \n",
            msg.objectID,
            msg.objectTypeID,
            msg.x_trans_m, msg.y_trans_m, msg.z_trans_m,
            msg.rot_deg, msg.rot_axis_x, msg.rot_axis_y, msg.rot_axis_z,
            msg.color);
      
      objectMap_[msg.objectID] = msg;
    }
    
    
    void ProcessVizEraseObjectMessage(const VizEraseObject& msg)
    {
      PRINT("Processing EraseObject\n");
      
      if (msg.objectID == ALL_OBJECT_IDs) {
        objectMap_.clear();
      } else {
        objectMap_.erase(msg.objectID);
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
      float dx,dy,cosCurrRad;
      //dWebotsConsolePrintf("***** ARC rad %f (%f to %f), radius %f\n", currRad, startRad, endRad, radius);
      
      while (currRad*dir < endRad*dir) {
        cosCurrRad = cos(currRad);
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

void webots_physics_init(dWorldID world, dSpaceID space, dJointGroupID contactJointGroup) {
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
  
  server.StartListening(Anki::Cozmo::VIZ_SERVER_PORT);
  
  pathMap_.clear();
}

void webots_physics_step() {
  /*
   * Do here what needs to be done at every time step, e.g. add forces to bodies
   *   dBodyAddForce(body1, f[0], f[1], f[2]);
   *   ...
   */

  // Process draw messages from CozmoBot
  int msgSize;
  float *msgBuf = (float*)dWebotsReceive(&msgSize);
  float *msg = msgBuf;
  int msgOffset = 0;
  int robotID;
  int pathID;
  PathVertex_t pt;
  pt.resize(3);


  // Process messages from basestation
  int bytes_recvd = 0;
  const int MAX_RECV_BUF_SIZE = 1024;
  u8 recvBuf[MAX_RECV_BUF_SIZE];
  while ((bytes_recvd = server.Recv((char*)recvBuf, MAX_RECV_BUF_SIZE)) > 0) {
    int msgID = static_cast<Anki::Cozmo::VizMsgID>(recvBuf[0]);
    PRINT( "CozmoPhysics: Got msg %d (%d bytes)\n", msgID, bytes_recvd);
    
    (*Anki::Cozmo::LookupTable_[msgID].dispatchFcn)(recvBuf + 1);
  }
  
  
  // TODO: Get rid of everything below here once sim_pathFollower has been converted to use the same messaging interface.
  //       Also rename sim_pathFollower to VizController or something...
  
  if (msgBuf) {
    //dWebotsConsolePrintf("***** MSG RECVD: %d\n", msgSize);
    while (msgOffset*SIZEOF_FLOAT < msgSize) {
      
      msg = msgBuf + msgOffset;
      //dWebotsConsolePrintf("***** MSGOFFSET: %d (%x, %x)\n", msgOffset, msgBuf, msg);

      switch ((int)msg[0]) {
      
      case PLUGIN_MSG_ERASE_PATH:
        robotID = msg[PLUGIN_MSG_ROBOT_ID];
        pathID = msg[PLUGIN_MSG_PATH_ID];
        pathMap_[pathID].clear();
        msgOffset += ERASE_PATH_MSG_SIZE;
        //dWebotsConsolePrintf("***** ERASE RECVD\n");
        break;

      case PLUGIN_MSG_APPEND_LINE:
        robotID = msg[PLUGIN_MSG_ROBOT_ID];
        pathID = msg[PLUGIN_MSG_PATH_ID];
        pt[0] = msg[LINE_START_X];
        pt[1] = msg[LINE_START_Y];
        pt[2] = heightOffset_;
        pathMap_[pathID].push_back(pt);
        pt[0] = msg[LINE_END_X];
        pt[1] = msg[LINE_END_Y];
        pt[2] = heightOffset_;
        pathMap_[pathID].push_back(pt);
        msgOffset += LINE_MSG_SIZE;
        //dWebotsConsolePrintf("***** LINE RECVD (robot %d, path %d, totalPathPoints %d)\n", robotID, pathID, robotPathMap[robotID][pathID].size());
        break;

      case PLUGIN_MSG_APPEND_ARC:
        {
          
          robotID = msg[PLUGIN_MSG_ROBOT_ID];
          pathID = msg[PLUGIN_MSG_PATH_ID];

          
          float center_x = msg[ARC_CENTER_X];
          float center_y = msg[ARC_CENTER_Y];
          float center_z = heightOffset_;

          float radius = msg[ARC_RADIUS];
          float startRad = msg[ARC_START_RAD];
          float sweepRad = msg[ARC_SWEEP_RAD];
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
          float dx,dy,cosCurrRad;
          //dWebotsConsolePrintf("***** ARC rad %f (%f to %f), radius %f\n", currRad, startRad, endRad, radius);

          while (currRad*dir < endRad*dir) {
            cosCurrRad = cos(currRad);
            dx = cos(currRad) * radius;
            dy = sin(currRad) * radius;
            pt[0] = center_x + dx;
            pt[1] = center_y + dy;
            pt[2] = center_z;
            pathMap_[pathID].push_back(pt);
            currRad += dir * arcRes_rad;
          }
           
          msgOffset += ARC_MSG_SIZE;
          break;
        }

      case PLUGIN_MSG_SHOW_PATH:
        // Currently, this message applies globally
        drawPaths_ = msg[SHOW_PATH];
        msgOffset += SHOW_PATH_MSG_SIZE;
        break;

      case PLUGIN_MSG_SET_HEIGHT_OFFSET:
        // Currently, this message applies globally
        heightOffset_ = msg[HEIGHT_OFFSET];
        msgOffset += SET_HEIGHT_OFFSET_MSG_SIZE;
        break;

      default:
        // It's possible for this to receive messages that were sent to CHANNEL_BROADCAST
        // so just ignore them.
        //dWebotsConsolePrintf("***ERROR: UNKNOWN MSG RECVD BY PLUGIN %d\n", msgSize);
        return;
        break;
      }
    }
  }

}


void draw_cuboid(float x, float y, float z)
{
  
   // Webots hack
  float halfX = x*0.5;
  float halfY = y*0.5;
  float halfZ = z*0.5;
 
  
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


void webots_physics_draw(int pass, const char *view) {
 
  // Only draw in main 3D view (view == NULL) and not the camera views
  if (pass == 1 && view == NULL) {
    
    // setup draw style
    glDisable(GL_LIGHTING);
    glLineWidth(2);

    // Set default color
    glColor3f(DEFAULT_COLOR[0], DEFAULT_COLOR[1], DEFAULT_COLOR[2]);
    
    // Draw path
    PathMap_t::iterator pathMapIt;
    for (pathMapIt = pathMap_.begin(); pathMapIt != pathMap_.end(); pathMapIt++) {

      int pathID = pathMapIt->first;
      
      // Set path color
      if (pathColorMap_.find(pathID) != pathColorMap_.end()) {
        VizColorDef_t::iterator cIt = colorMap_.find(pathColorMap_[pathID]);
        if (cIt != colorMap_.end()) {
          Anki::Cozmo::VizDefineColor *c = &(cIt->second);
          glColor3f(c->r, c->g, c->b);
        }
      }
      
      // Draw the path
      glBegin(GL_LINE_STRIP);
      
      for (Path_t::iterator pathIt = pathMapIt->second.begin(); pathIt != pathMapIt->second.end(); pathIt++) {
//        glVertex3f((*pathIt)[0],(*pathIt)[2],-(*pathIt)[1]); // Map to simulation world coords
        glVertex3f((*pathIt)[0],(*pathIt)[1],(*pathIt)[2] + heightOffset_);
      }
      glEnd();

      // Restore default color
      glColor3f(DEFAULT_COLOR[0], DEFAULT_COLOR[1], DEFAULT_COLOR[2]);
    }
    
    
    // Draw objects
    VizObject_t::iterator objIt;
    for (objIt = objectMap_.begin(); objIt != objectMap_.end(); ++objIt) {
     
      Anki::Cozmo::VizObject *obj = &(objIt->second);
      
      // Set color for the block
      VizColorDef_t::iterator cIt = colorMap_.find(obj->color);
      if (cIt != colorMap_.end()) {
        Anki::Cozmo::VizDefineColor *c = &(cIt->second);
        glColor3f(c->r, c->g, c->b);
      }
      
      // Set pose
      glPushMatrix();
      
      glTranslatef(obj->x_trans_m, obj->y_trans_m, obj->z_trans_m);
      glRotatef(obj->rot_deg, obj->rot_axis_x, obj->rot_axis_y, obj->rot_axis_z);

      // For now, all objects are just bounding boxes
      draw_cuboid(obj->x_size_m, obj->y_size_m, obj->z_size_m);
      
      // TODO: use objectType-specific drawing functions
      /*
      switch(obj->objectTypeID) {
        case 0:
          
          break;
        default:
          PRINT("Unknown objectTypeID %d\n", obj->objectTypeID);
          break;
      }
       */
      
      glPopMatrix();
      
      // Restore default color
      glColor3f(DEFAULT_COLOR[0], DEFAULT_COLOR[1], DEFAULT_COLOR[2]);
    }
    
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
}






