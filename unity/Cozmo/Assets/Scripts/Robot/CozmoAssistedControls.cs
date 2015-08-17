using UnityEngine;
using System.Collections;

/// <summary>
/// some research into adjusting driving controls dynamically to avoid obstacles of which cozmo is aware
/// </summary>
public class CozmoAssistedControls : MonoBehaviour
{
  public static CozmoAssistedControls instance;
  [SerializeField] float lookAheadSeconds = 0.5f;
  [SerializeField] float detectRange = 100;
  [SerializeField] float halfWidth = 11;
  [SerializeField] float upperLength = 22;
  [SerializeField] float lowerLength = 30;
  [SerializeField] float debugLeft = 0;
  [SerializeField] float debugRight = 0;
  Robot robot = null;
  Vector3[] objNormals;
  Vector3[] robotVerts;
  float[] projectedRobotVerts;
  float[] wheelSpeeds;
  float[] lastWheelSpeeds;
  bool sawCollsionLastFrame = false;
  float potentialCollisionTime = 0;

  const float max_wheel_speed = 160;

  void Awake()
  {
    instance = this;
    potentialCollisionTime = float.MinValue;
  }

  void Start()
  {
    robot = RobotEngineManager.instance.current;
    objNormals = new Vector3[4];
    robotVerts = new Vector3[4];
    projectedRobotVerts = new float[4];
    wheelSpeeds = new float[2];
    lastWheelSpeeds = new float[2];
  }

  public float[] AdjustForCollsion(float left_speed, float right_speed)
  {
    wheelSpeeds[0] = left_speed;
    wheelSpeeds[1] = right_speed;

    if(left_speed == 0 && right_speed == 0)
    {
      sawCollsionLastFrame = false;
      return wheelSpeeds;
    }
      
    Vector3 heading = lookAheadSeconds * (left_speed * robot.Right - right_speed * robot.Right + (left_speed + right_speed) * robot.Forward);
    Vector3 pos = robot.WorldPosition + heading;
    Vector3 heading_norm = heading.normalized;
    Vector3 heading_right = Vector3.Cross(heading_norm, Vector3.forward);

    robotVerts[0] = pos + upperLength * heading_norm - halfWidth * heading_right;
    robotVerts[1] = pos + upperLength * heading_norm + halfWidth * heading_right;
    robotVerts[2] = pos - lowerLength * heading_norm - halfWidth * heading_right;
    robotVerts[3] = pos - lowerLength * heading_norm + halfWidth * heading_right;

    //wheelSpeeds[0] = debugLeft;
    //wheelSpeeds[1] = debugRight;

/*
    if (vectorSet)
    {
      //Vector3.
      Vector2 turnVector = new Vector2(robot.Right.x, robot.Right.y);
      Vector2 robot_pos2 = new Vector2(robot.WorldPosition.x, robot.WorldPosition.y);
      Vector2 point1 = robot_pos2 + 100*turnVector;
      Vector2 point2 = robot_pos2 - 100*turnVector;

      // get our line equations

      float A1 = point1.y - point2.y;
      float B1 = point2.x - point1.x;
      float C1 = A1*point2.x + B1*point2.y;

      float A2 = lastTurnPoints[0].y - lastTurnPoints[1].y;
      float B2 = lastTurnPoints[1].x - lastTurnPoints[0].x;
      float C2 = A2*lastTurnPoints[1].x + B2*lastTurnPoints[1].y;

      float determinate = A1*B2 - A2*B1;
      if(determinate != 0)
      {      
        float x = (B2*C1 - B1*C2)/determinate;
        float y = (A1*C2 - A2*C1)/determinate;
        //RobotEngineManager.instance.VisualizeQuad(29, CozmoPalette.ColorToUInt(Color.blue), new Vector3(x-10,y+10,0), new Vector3(x+10,y+10,0),new Vector3(x+10,y-10,0), new Vector3(x-10,y-10,0));
      //  RobotEngineManager.instance.VisualizeQuad(counter, CozmoPalette.ColorToUInt(Color.red), new Vector3(point1.x,point1.y,0), new Vector3(point1.x,point1.y,0),new Vector3(point2.x,point2.y,0), new Vector3(point2.x,point2.y,0));
        counter++;
        //Debug.Log("radius: " + (robot.WorldPosition-new Vector3(x,y,0)).magnitude);
      }

      lastTurnPoints[0] = point1;
      lastTurnPoints[1] = point2;

    }
    else
    {
      vectorSet = true;
      Vector2 turnVector = new Vector2(robot.Right.x, robot.Right.y);
      Vector2 robot_pos2 = new Vector2(robot.WorldPosition.x, robot.WorldPosition.y);
      Vector2 point1 = robot_pos2 + 1000*turnVector;
      Vector2 point2 = robot_pos2 - 1000*turnVector;
      lastTurnPoints[0] = point1;
      lastTurnPoints[1] = point2;
    }

    */



    Vector3 left = pos - 5 * heading_right;
    Vector3 right = pos + 5 * heading_right;
    RobotEngineManager.instance.VisualizeQuad(25, CozmoPalette.ColorToUInt(Color.blue), left, right, right + heading, left + heading);



    for(int i = 0; i < robot.knownObjects.Count; i++)
    {
      ObservedObject obj = robot.knownObjects[i];
      if(Vector3.Magnitude(robot.WorldPosition - obj.WorldPosition) < detectRange)
      {
        bool collided = true;

        objNormals[0] = obj.Right; // right
        objNormals[1] = -obj.Right; // left
        objNormals[2] = obj.Forward; // forward
        objNormals[3] = -obj.Forward; // back

        // check the four sides for collision
        Vector3 min_overlap_axis = new Vector3();
        float min_overlap = float.MaxValue;

        for(int side = 0; side < 4; side++)
        {
          float obj_min = Vector3.Dot(objNormals[side], obj.WorldPosition - 22 * objNormals[side]); 
          float obj_max = Vector3.Dot(objNormals[side], obj.WorldPosition + 22 * objNormals[side]); 

          // project the robot's verts onto the normal
          projectedRobotVerts[0] = Vector3.Dot(objNormals[side], robotVerts[0]);
          projectedRobotVerts[1] = Vector3.Dot(objNormals[side], robotVerts[1]);
          projectedRobotVerts[2] = Vector3.Dot(objNormals[side], robotVerts[2]);
          projectedRobotVerts[3] = Vector3.Dot(objNormals[side], robotVerts[3]);

          float min = float.MaxValue;
          float max = float.MinValue;

          for(int vert = 0; vert < 4; vert++)
          {
            if(projectedRobotVerts[vert] < min) min = projectedRobotVerts[vert];

            if(projectedRobotVerts[vert] > max) max = projectedRobotVerts[vert];
          }

          if(max < obj_min || min > obj_max)
          {
            collided = false;
            break;
          } else
          {
            // get the amount of overlap
            // obj_min       obj_max
            // ^             ^
            // ***************
            //           min          max
            //           ^            ^
            //           **************
            float overlap = (max - obj_min) - (max - obj_max) - (min - obj_min);
            if(overlap < min_overlap)
            {
              min_overlap = overlap;
              min_overlap_axis = objNormals[side];
            }
          }

        }

        if(collided && potentialCollisionTime < Time.realtimeSinceStartup)
        {
          // make our adjustements
          RobotEngineManager.instance.VisualizeQuad(24, CozmoPalette.ColorToUInt(Color.red), robotVerts[0], robotVerts[1], robotVerts[3], robotVerts[2]);
          Debug.LogWarning("min_overlap: " + min_overlap + ", normal: " + min_overlap_axis);
          if(!sawCollsionLastFrame)
          {
            sawCollsionLastFrame = true;
            potentialCollisionTime = Time.realtimeSinceStartup + lookAheadSeconds;
          }
          // find our easiest correction, and adjust our heading there
          float timeToCollsion = potentialCollisionTime - Time.realtimeSinceStartup;

          Vector3 newDesiredHeading;

          heading = timeToCollsion * (left_speed * robot.Right - right_speed * robot.Right + (left_speed + right_speed) * robot.Forward);
          Vector3 circle_center = new Vector3();

          if(Vector2.Dot(heading_right, min_overlap_axis) > 0)
          {
            // need to move right, so slow right treads
            newDesiredHeading = heading + min_overlap * heading_right;

            Vector3 new_desired_pos = newDesiredHeading + robot.WorldPosition;
            
            GetCircleOriginForPoints(new_desired_pos, out circle_center);
            float radius = (circle_center - robot.WorldPosition).magnitude;
            Debug.Log("to_center: " + radius);
            float new_right_speed = (left_speed / (23.25f + radius)) * (radius - 23.25f);

            wheelSpeeds[1] = new_right_speed;
          } else
          {
            // need to move left, so slow left treads
            newDesiredHeading = heading - min_overlap * heading_right;
            Vector3 new_desired_pos = newDesiredHeading + robot.WorldPosition;

            GetCircleOriginForPoints(new_desired_pos, out circle_center);
            float radius = (circle_center - robot.WorldPosition).magnitude;
            Debug.Log("to_center: " + radius);

            float new_left_speed = (right_speed / (23.25f + radius)) * (radius - 23.25f);

            wheelSpeeds[0] = new_left_speed;

          }

          lastWheelSpeeds = wheelSpeeds;


        } else if(potentialCollisionTime >= Time.realtimeSinceStartup)
        {
          Debug.Log("saw collision, still in tweak window, potentialCollisionTime: " + potentialCollisionTime + ", realtimeSinceStartup:" + Time.realtimeSinceStartup);
          // stay on course
          wheelSpeeds = lastWheelSpeeds;
          RobotEngineManager.instance.VisualizeQuad(24, CozmoPalette.ColorToUInt(Color.blue), robotVerts[0], robotVerts[1], robotVerts[3], robotVerts[2]);
        } else if(collided)
        {
          Debug.Log("saw collision, outside tweak window, potentialCollisionTime: " + potentialCollisionTime + ", realtimeSinceStartup:" + Time.realtimeSinceStartup);
        } else
        {
          sawCollsionLastFrame = false;
        }
        Debug.Log("wheelSpeeds[0]: " + wheelSpeeds[0] + ", wheelSpeeds[1]: " + wheelSpeeds[1]);
      }
    }

    return wheelSpeeds;
  }

  #region helpers

  void GetCircleOriginForPoints(Vector3 desired_position, out Vector3 circle_center)
  {
    circle_center = Vector3.zero;
    Vector3 robot_pos = robot.WorldPosition;
    float m = Mathf.Tan(robot.poseAngle_rad + (Mathf.PI / 2));
    float b = robot_pos.y - m * robot_pos.x;
    float x_r_2 = Mathf.Pow(robot_pos.x, 2);
    float y_r_2 = Mathf.Pow(robot_pos.y, 2);
    float x_d_2 = Mathf.Pow(desired_position.x, 2);
    float y_d_2 = Mathf.Pow(desired_position.y, 2);

    // y_o = (m * c0) + (m * c1)*y_o + b
    // => y_o = ((m * c0) + b)/(1-(m * c1))
    //
    // where c0 = (x_r_2 - x_d_2 + y_r_2 - y_d_2)/(2*x_r - 2*x_d)
    // and c1 = (2*y_d - 2*y_r)/(2*x_r - 2*x_d)
    float c_denom = 2 * robot_pos.x - 2 * desired_position.x;

    if(c_denom != 0)
    {
      float c0 = (x_r_2 - x_d_2 + y_r_2 - y_d_2) / (c_denom);
      float c1 = (2 * desired_position.y - 2 * robot_pos.y) / (c_denom);

      circle_center.y = ((m * c0) + b) / (1 - (m * c1));
      circle_center.x = c0 + c1 * circle_center.y;
    }

    RobotEngineManager.instance.VisualizeQuad(29, CozmoPalette.ColorToUInt(Color.blue), new Vector3(circle_center.x - 10, circle_center.y + 10, 0), new Vector3(circle_center.x + 10, circle_center.y + 10, 0), new Vector3(circle_center.x + 10, circle_center.y - 10, 0), new Vector3(circle_center.x - 10, circle_center.y - 10, 0));
    RobotEngineManager.instance.VisualizeQuad(30, CozmoPalette.ColorToUInt(Color.red), new Vector3(desired_position.x - 10, desired_position.y + 10, 0), new Vector3(desired_position.x + 10, desired_position.y + 10, 0), new Vector3(desired_position.x + 10, desired_position.y - 10, 0), new Vector3(desired_position.x - 10, desired_position.y - 10, 0));
  }

  #endregion
}
