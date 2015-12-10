using UnityEngine;
using System.Collections;

namespace FaceTracking {
  public class FaceTrackingGameConfig : MinigameConfigBase {

    public int Goal;
    public float TiltTreshold;
    public float TurnSpeed;
    public float Lenience;
    public float MaxFaceDistance;
    public float MinFaceDistance;
    public float FaceJumpLimit;
    public bool WanderEnabled;
  
  }
}
