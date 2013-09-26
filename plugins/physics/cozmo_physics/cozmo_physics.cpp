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

// Types for paths
using namespace std;
typedef vector<float> PathVertex_t;
typedef vector<PathVertex_t> Path_t;
typedef map<int, Path_t > PathMap_t;
typedef map<int, PathMap_t> RobotPathMap_t;

// Map of all paths indexed by robotID and pathID
static RobotPathMap_t robotPathMap;

// Whether or not to draw paths
static bool drawPaths_ = true;

// Default height offset of paths (m)
static float heightOffset_ = 0.1;

// Default angular resolution of arc path segments (radians)
static float arcRes_rad = 0.2;

const int SIZEOF_FLOAT = sizeof(float);

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

  robotPathMap.clear();
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

  if (msgBuf) {
    //dWebotsConsolePrintf("***** MSG RECVD: %d\n", msgSize);
    while (msgOffset*SIZEOF_FLOAT < msgSize) {
      
      msg = msgBuf + msgOffset;
      //dWebotsConsolePrintf("***** MSGOFFSET: %d (%x, %x)\n", msgOffset, msgBuf, msg);

      switch ((int)msg[0]) {
      
      case PLUGIN_MSG_ERASE_PATH:
        robotID = msg[PLUGIN_MSG_ROBOT_ID];
        pathID = msg[PLUGIN_MSG_PATH_ID];
        robotPathMap[robotID][pathID].clear();
        msgOffset += ERASE_PATH_MSG_SIZE;
        //dWebotsConsolePrintf("***** ERASE RECVD\n");
        break;

      case PLUGIN_MSG_APPEND_LINE:
        robotID = msg[PLUGIN_MSG_ROBOT_ID];
        pathID = msg[PLUGIN_MSG_PATH_ID];
        pt[0] = msg[LINE_START_X];
        pt[1] = msg[LINE_START_Y];
        pt[2] = heightOffset_;
        robotPathMap[robotID][pathID].push_back(pt);
        pt[0] = msg[LINE_END_X];
        pt[1] = msg[LINE_END_Y];
        pt[2] = heightOffset_;
        robotPathMap[robotID][pathID].push_back(pt);
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
          float endRad = msg[ARC_END_RAD];

          if (endRad < startRad) {
            // Arc sweeps CW
            // Swap start and end so that we can draw it the same way
            float tempRad = endRad;
            endRad = startRad;
            startRad = tempRad;
          } 

          // Add points along arc from startRad to endRad at arcRes_rad resolution
          float currRad = startRad;
          float dx,dy,cosCurrRad;
          //dWebotsConsolePrintf("***** ARC rad %f (%f to %f), radius %f\n", currRad, startRad, endRad, radius);

          while (currRad < endRad) {
            cosCurrRad = cos(currRad);
            dx = cosCurrRad * radius;
            dy = sqrt(radius*radius - dx*dx) * (currRad > 0 ? 1 : -1);
            pt[0] = center_x + dx;
            pt[1] = center_y + dy;
            pt[2] = center_z;
            robotPathMap[robotID][pathID].push_back(pt);
            currRad += arcRes_rad;
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
        dWebotsConsolePrintf("***ERROR: UNKNOWN MSG RECVD BY PLUGIN %d\n", msgSize);
        return;
        break;
      }
    }
  }

}

void webots_physics_draw() {
  
   // setup draw style
   glDisable(GL_LIGHTING);
   glLineWidth(2);

   RobotPathMap_t::iterator robotPathMapIt;
   for (robotPathMapIt = robotPathMap.begin(); robotPathMapIt != robotPathMap.end(); robotPathMapIt++) {
     PathMap_t::iterator pathMapIt;
     int robotID = robotPathMapIt->first;
     for (pathMapIt = robotPathMapIt->second.begin(); pathMapIt != robotPathMapIt->second.end(); pathMapIt++) {
       Path_t::iterator pathIt;
       int pathID = pathMapIt->first;
        
       // Draw the path
       glBegin(GL_LINE_STRIP);

       // Set path color
       if (pathID == 0) {
         // Commanded path
         glColor3f(1, 1, 0);
       } else if (pathID == 1) { 
         // Actual path traversed
         glColor3f(0, 1, 0);
       } else {
         glColor3f(1, 0, 0);
       }

       for (pathIt = pathMapIt->second.begin(); pathIt != pathMapIt->second.end(); pathIt++) {
         glVertex3f((*pathIt)[0],(*pathIt)[2],-(*pathIt)[1]); // Map to simulation world coords
       }
       glEnd();

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

  robotPathMap.clear();
}
