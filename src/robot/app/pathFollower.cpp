#include "app/pathFollower.h"
#include "app/localization.h"
#include "app/debug.h"
#include "app/vehicleMath.h"
#include <stdio.h>


#define ENABLE_PATH_VIZ 1

#if(ENABLE_PATH_VIZ)
#include "cozmoBot.h"
extern CozmoBot gCozmoBot;
#endif


namespace PathFollower
{

namespace {
  typedef enum {
    PST_LINE,
    PST_ARC
  } PathSegmentType;

  typedef struct 
  {
    PathSegmentType type;

    // Line
    Anki::Embedded::Point2f startPt;
    Anki::Embedded::Point2f endPt;
    float m;  // slope of line
    float b;  // y-intersect of line
    float dy_sign; // 1 if endPt.y - startPt.y >= 0
    Anki::Radians theta; // angle of line
    // TODO: Speed profile (by distance along line)


    // Arc
    Anki::Embedded::Point2f centerPt;
    float radius;
    Anki::Radians startRad;
    Anki::Radians endRad;
    BOOL movingCCW;

  } PathSegment;

  #define MAX_NUM_PATH_SEGMENTS 10
  PathSegment Path[MAX_NUM_PATH_SEGMENTS];
  s16 numPathSegments_ = 0;
  s16 currPathSegment_ = -1;

  BOOL visualizePath_ = TRUE;
}


void InitPathFollower(void)
{
  ClearPath();
}


// Deletes current path
void ClearPath(void)
{
  numPathSegments_ = 0;
#if(ENABLE_PATH_VIZ)
  gCozmoBot.ErasePath(0);
#endif
}

void EnablePathVisualization(BOOL on)
{
  visualizePath_ = on;
}


// Add path segment
// tODO: Change units to meters
BOOL AppendPathSegment_Line(u32 matID, float x_start_m, float y_start_m, float x_end_m, float y_end_m)
{
  if (numPathSegments_ >= MAX_NUM_PATH_SEGMENTS) {
    return FALSE;
  }

  Path[numPathSegments_].type = PST_LINE;
  Path[numPathSegments_].startPt.x = x_start_m;
  Path[numPathSegments_].startPt.y = y_start_m;
  Path[numPathSegments_].endPt.x = x_end_m;
  Path[numPathSegments_].endPt.y = y_end_m;

  // Pre-processed for convenience
  Path[numPathSegments_].m = (y_end_m - y_start_m) / (x_end_m - x_start_m);
  Path[numPathSegments_].b = Path[numPathSegments_].m * y_start_m + x_start_m; 
  Path[numPathSegments_].dy_sign = ((y_end_m - y_start_m) >= 0) ? 1.0 : -1.0;
  Path[numPathSegments_].theta = atan2(y_end_m - y_start_m, x_end_m - x_start_m);

  numPathSegments_++;

#if(ENABLE_PATH_VIZ)
  gCozmoBot.AppendPathSegmentLine(0, x_start_m, y_start_m, x_end_m, y_end_m);
#endif

  return TRUE;
}


BOOL AppendPathSegment_Arc(u32 matID, float x_center_m, float y_center_m, float radius_m, float startRad, float endRad)
{
  if (numPathSegments_ >= MAX_NUM_PATH_SEGMENTS) {
    return FALSE;
  }

  // Assuming arcs are true functions.
  // i.e. for any value of x there can only be one y value.
  // This also means that an arc cannot sweep past the angles 0 or PI.
  assert( (SIGN(startRad) == SIGN(endRad)) || 
          (FLT_NEAR(startRad,0) || FLT_NEAR(endRad,0)) 
        );

  Path[numPathSegments_].type = PST_ARC;
  Path[numPathSegments_].centerPt.x = x_center_m;
  Path[numPathSegments_].centerPt.y = y_center_m;
  Path[numPathSegments_].radius = radius_m;
  Path[numPathSegments_].startRad = startRad;
  Path[numPathSegments_].endRad = endRad;

  // Pre-processed for convenience
  Path[numPathSegments_].movingCCW = endRad > startRad;

  numPathSegments_++;

#if(ENABLE_PATH_VIZ)
  gCozmoBot.AppendPathSegmentArc(0, x_center_m, y_center_m, radius_m, startRad, endRad);
#endif

  return TRUE;
}




int GetNumPathSegments(void)
{
  return numPathSegments_;
}


BOOL StartPathTraversal()
{
  // Set first path segment
  if (numPathSegments_ > 0) {
    currPathSegment_ = 0;
  }

  // Visualize path
  if (visualizePath_) {
#if(ENABLE_PATH_VIZ)
    gCozmoBot.ShowPath(0, true);
#endif
  }

  return TRUE;
}


BOOL IsTraversingPath()
{
  return currPathSegment_ >= 0;
}


BOOL ProcessPathSegmentLine(float &shortestDistanceToPath_m, float &radDiff)
{
  PathSegment* currSeg = &(Path[currPathSegment_]);

  // Find shortest path to current segment.
  // Shortest path is along a line with inverse negative slope (i.e. -1/m).
  // Point of intersection is solution to mx + b == (-1/m)*x + b_inv where b_inv = y-(-1/m)*x
  Anki::Embedded::Pose2d currPose = Localization::GetCurrMatPose();

  float x = currPose.get_x();
  float y = currPose.get_y();

#if(DEBUG_PATH_FOLLOWER)
  printf("currPathSeg: %d, LINE (%f, %f, %f, %f)\n", currPathSegment_, currSeg->startPt.x, currSeg->startPt.y, currSeg->endPt.x, currSeg->endPt.y);
  printf("x: %f, y: %f\n", x,y);
#endif

  if (FLT_NEAR(currSeg->startPt.x, currSeg->endPt.x)) {
    // Special case: Vertical line
    if (currSeg->endPt.y > currSeg->startPt.y) {
      shortestDistanceToPath_m = currSeg->startPt.x - x;
    } else {
      shortestDistanceToPath_m = x - currSeg->startPt.x;
    }

    // Compute angle difference
    radDiff = (currSeg->theta - currPose.get_angle()).ToFloat();


    // Did we pass the current segment?
    // If the point (x_intersect,y_intersect) is not between startPt and endPt, then we've passed this segment and should
    // go to next one
    if ( SIGN(currSeg->startPt.y - y) == SIGN(currSeg->endPt.y - y) ) {
      return FALSE;
    }

  } else {

    float m = currSeg->m;
    float b = currSeg->b;

    float b_inv = y + x/m;

    float x_intersect = m * (b_inv - b) / (m*m + 1);
    float y_intersect = - (x_intersect / m) + b_inv;

    float dy = y - y_intersect;
    float dx = x - x_intersect;

    shortestDistanceToPath_m = sqrt(dy*dy + dx*dx);

#if(DEBUG_PATH_FOLLOWER)
    printf("m: %f, b: %f\n",m,b);
    printf("x_int: %f, y_int: %f, b_inv: %f\n", x_intersect, y_intersect, b_inv);
    printf("dy: %f, dx: %f, dist: %f\n", dy, dx, shortestDistanceToPath_m);
    printf("SIGN(dy): %d, dy_sign: %f\n", (SIGN(dy) ? 1 : -1), currSeg->dy_sign);
    printf("lineTheta: %f, robotTheta: %f\n", currSeg->theta.ToFloat(), currPose.get_angle().ToFloat());
#endif

    if (m >= 0) {
      shortestDistanceToPath_m *= (SIGN(dy) ? 1 : -1) * currSeg->dy_sign;
    } else {
      shortestDistanceToPath_m *= (SIGN(dy) ? -1 : 1) * currSeg->dy_sign;
    }
  

    // Compute angle difference
    radDiff = (currSeg->theta - currPose.get_angle()).ToFloat();


    // Did we pass the current segment?
    // If the point (x_intersect,y_intersect) is not between startPt and endPt, then we've passed this segment and should
    // go to next one
    if ( (SIGN(currSeg->startPt.x - x_intersect) == SIGN(currSeg->endPt.x - x_intersect)) ||
         (SIGN(currSeg->startPt.y - y_intersect) == SIGN(currSeg->endPt.y - y_intersect)) ) {
      return FALSE;
    }
  }

  return TRUE;
}


BOOL ProcessPathSegmentArc(float &shortestDistanceToPath_m, float &radDiff)
{
  PathSegment* currSeg = &(Path[currPathSegment_]);

#if(DEBUG_PATH_FOLLOWER)
  printf("currPathSeg: %d, ARC (%f, %f), startRad: %f, endRad: %f, radius: %f\n", 
         currPathSegment_, currSeg->centerPt.x, currSeg->centerPt.y, currSeg->startRad.ToFloat(), currSeg->endRad.ToFloat(), currSeg->radius);
#endif

  Anki::Embedded::Pose2d currPose = Localization::GetCurrMatPose();

  float x = currPose.get_x();
  float y = currPose.get_y();


  // Assuming arc is broken up so that it is a true function

  // Arc paramters
  float x_center = currSeg->centerPt.x;
  float y_center = currSeg->centerPt.y;
  float r = currSeg->radius;
  Anki::Radians startRad = currSeg->startRad;
  Anki::Radians endRad = currSeg->endRad;

  // Line formed by circle center and robot pose
  float dy = y - y_center;
  float dx = x - x_center;
  float m = dy / dx;
  float b = y - m*x;


  // Where does circle (x - x_center)^2 + (y - y_center)^2 = r^2 and y=mx+b intersect where y=mx+b represents 
  // the line between the circle center and the robot?
  // (y - y_center)^2 == r^2 - (x - x_center)^2
  // y = sqrt(r^2 - (x - x_center)^2) + y_center
  //   = mx + b
  // (mx + b - y_center)^2 == r^2 - (x - x_center)^2
  // m^2*x^2 + 2*m*(b - y_center)*x + (b - y_center)^2 == r^2 - x^2 + 2*x_center*x - x_center^2
  // (m^2+1) * x^2 + (2*m*(b - y_center) - 2*x_center) * x + (b - y_center)^2 - r^2 + x_center^2 == 0
  //
  // Use quadratic formula to solve

  // Quadratic formula coefficients
  float A = m*m + 1;
  float B = 2*m*(b-y_center) - 2*x_center;
  float C = (b - y_center)*(b - y_center) - r*r + x_center*x_center;
  float sqrtPart = sqrt(B*B - 4*A*C);

  float x_intersect_1 = (-B + sqrtPart) / (2*A);
  float x_intersect_2 = (-B - sqrtPart) / (2*A);


  // Now we have 2 roots.
  // Select the one that's on the same side of the circle center as the robot is.
  float x_intersect = x_intersect_2;
  if (SIGN(x - x_center) == SIGN(x_intersect_1 - x_center)) {
    x_intersect = x_intersect_1;
  }

  float y_intersect = x_intersect * m + b;
  float y_int_alt = sqrt(r*r - x_intersect * x_intersect);

  shortestDistanceToPath_m = sqrt((x - x_intersect) * (x - x_intersect) + (y - y_intersect) * (y - y_intersect));

  float arcSweep = (currSeg->endRad - currSeg->startRad).ToFloat();
  BOOL robotInsideCircle = ABS(dx) < ABS(x_intersect - x_center);
  if ((robotInsideCircle && arcSweep < 0) || (!robotInsideCircle && arcSweep > 0)) {
    shortestDistanceToPath_m *= -1;
  }


  // Find heading error
  Anki::Radians theta_line = atan2(dy,dx); // angle of line from circle center to robot
  Anki::Radians theta_tangent = theta_line + Anki::Radians((currSeg->movingCCW ? 1 : -1 ) * PIDIV2);
  
  radDiff = (theta_tangent - currPose.get_angle()).ToFloat();

#if(DEBUG_PATH_FOLLOWER)
  printf("x: %f, y: %f, m: %f, b: %f\n", x,y,m,b);
  printf("x_center: %f, y_center: %f\n", x_center, y_center);
  printf("x_int: %f, y_int: %f, x's: (%f %f), alternate y_int: %f\n", x_intersect, y_intersect, x_intersect_1, x_intersect_2, y_int_alt);
  printf("dy: %f, dx: %f, dist: %f\n", dy, dx, shortestDistanceToPath_m);
  printf("arcSweep: %f, insideCircle: %d\n", arcSweep, robotInsideCircle);
  printf("theta_line: %f, theta_tangent: %f\n", theta_line.ToFloat(), theta_tangent.ToFloat());
#endif

  // Did we pass the current segment?
  if ( (currSeg->movingCCW && (theta_line - currSeg->startRad > arcSweep)) || 
       (!currSeg->movingCCW && (theta_line - currSeg->startRad < arcSweep)) ){
    return FALSE;
  }


  return TRUE;
}


BOOL GetPathError(float &shortestDistanceToPath_m, float &radDiff)
{
  if (currPathSegment_ < 0) {
    return FALSE;
  }

  static BOOL wasInRange = FALSE;
  BOOL inRange;

  switch (Path[currPathSegment_].type) {
  case PST_LINE:
    inRange = ProcessPathSegmentLine(shortestDistanceToPath_m, radDiff);
    break;
  case PST_ARC:
    inRange = ProcessPathSegmentArc(shortestDistanceToPath_m, radDiff);
    break;
  default:
    break;
  }

  // Go to next path segment if no longer in range of the current one
  if (!inRange && wasInRange) {
    if (++currPathSegment_ >= numPathSegments_) {
      // Path is complete
      currPathSegment_ = -1;
#if(DEBUG_PATH_FOLLOWER)
      printf("PATH COMPLETE\n");
#endif
#if(ENABLE_PATH_VIZ)
      gCozmoBot.ErasePath(0);
#endif
      return FALSE;
    }
  }

  wasInRange = inRange;


  // Check that starting error is not too big
  // TODO: Check for excessive heading error as well?
  if (shortestDistanceToPath_m > 0.05) {
    currPathSegment_ = -1;
#if(DEBUG_PATH_FOLLOWER)
    printf("PATH STARTING ERROR TOO LARGE %f\n", shortestDistanceToPath_m);
#endif
    return FALSE;
  }

  return TRUE;
}


}
