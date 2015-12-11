using UnityEngine;
using System.Collections;

namespace FaceTracking {
  public class FaceTrackingGameConfig : MinigameConfigBase {

    public float TiltTreshold;
    public float Lenience;
    public float MaxFaceDistance;
    public float MinFaceDistance;
    public float FaceJumpLimit;
    public bool WanderEnabled;
    public int MaxAttempts;
  
  }
}
