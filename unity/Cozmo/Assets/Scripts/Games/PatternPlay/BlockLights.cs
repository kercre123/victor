using UnityEngine;
using System.Collections;

// block lights relative to cozmo.
// front is the light facing cozmo.
// left is the light left of cozmo etc.
public struct BlockLights {
  public bool front;
  public bool back;
  public bool left;
  public bool right;
}
