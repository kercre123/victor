using UnityEngine;
using System.Collections;

namespace FaceTracking {
  /// <summary>
  /// Game for building face tracking skills. Has config options to enable Cozmo wandering
  /// when not paying attention to a face.
  /// TODO: A bunch of this logic can be seriously cleaned up once we can order Cozmo to track
  /// faces directly rather than handling all the movement by hand.
  /// </summary>
  public class FaceTrackingGame : GameBase {
    
    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();
    private int _TiltSuccessCount = 0;
    private int _TiltGoalTarget = 3;

    public float TiltGoal { get; set; }

    public float DistanceMin { get; set; }

    public float DistanceMax { get; set; }

    public float FaceJumpLimit { get; set; }

    public float GoalLenience { get; set; }

    public float StepsCompleted { get; set; }

    public bool WanderEnabled { get; set; }

    public bool TargetLeft { get; set; }

    public bool TargetRight { get; set; }

    public bool MidCelebration { get; set; }

    protected override void Initialize(MinigameConfigBase minigameConfigData) {
      FaceTrackingGameConfig config = (minigameConfigData as FaceTrackingGameConfig);
      _TiltGoalTarget = config.Goal;
      WanderEnabled = config.WanderEnabled;
      TiltGoal = config.TiltTreshold;
      GoalLenience = config.Lenience;
      TargetLeft = false;
      TargetRight = false;
      MidCelebration = false;
      DistanceMax = config.MaxFaceDistance;
      DistanceMin = config.MinFaceDistance;
      FaceJumpLimit = config.FaceJumpLimit;
      StepsCompleted = 0.0f;
      InitializeMinigameObjects();
    }

    // Calculates tilt based on the current face, triggers success if you've gotten to the right threshold
    public float CalculateTilt(Face face) {
      float angle = Vector3.Cross(CurrentRobot.Forward, face.WorldPosition - CurrentRobot.WorldPosition).z;
      // If the Angle is (< 0). Face is to your right, if (> 0) Face is to your left.
      float tiltVal = angle / TiltGoal;
      float goalVal = Mathf.Abs(tiltVal);

      // Determine success, or if we're even headed in the right direction, don't trigger success
      // if we are too far to the desired direction
      if (1.0f >= goalVal && goalVal >= (1.0f-GoalLenience) && !MidCelebration) {
        // Mark successful leading to each side.
        // TODO: Create a buildup delay so players have to hold for a short duration before triggering
        // a success.
        if (tiltVal > 0 && !TargetLeft) {
          TargetLeft = true;
          CurrentRobot.SetBackpackBarLED(Anki.Cozmo.LEDId.LED_BACKPACK_LEFT, Color.green);
          StepsCompleted += .333f;
          MidCelebration = true;
          CurrentRobot.SendAnimation(AnimationName.kEnjoyLight, HandleMiniCelebration);
        }
        else if (tiltVal < 0 && !TargetRight) {
          TargetRight = true;
          CurrentRobot.SetBackpackBarLED(Anki.Cozmo.LEDId.LED_BACKPACK_RIGHT, Color.green);
          StepsCompleted += .333f;
          MidCelebration = true;
          CurrentRobot.SendAnimation(AnimationName.kEnjoyLight, HandleMiniCelebration);
        }
        // Trigger a success if you've lit up both.
        if (TargetLeft && TargetRight) {
          TiltSuccess();
        }
      }

      return tiltVal;
    }

    public void TiltSuccess() {
      _TiltSuccessCount++;
      StepsCompleted = 1.0f;
      MidCelebration = true;
      CurrentRobot.SendAnimation(AnimationName.kFinishTapCubeWin, HandleEndCelebration);
    }

    protected void InitializeMinigameObjects() {
      
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("PeekGameStateMachine", _StateMachine);
      Progress = 0.0f;

      CurrentRobot.SetBehaviorSystem(true);
      CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);

      if (WanderEnabled) {
        CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.LookAround);
      }
      else {
        CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      }
      _StateMachine.SetNextState(new LookingForFaceState());

      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, true);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMotion, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingMarkers, false);
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
      if (Progress != StepsCompleted) {
        Progress = StepsCompleted;
      }
    }

    private void HandleMiniCelebration(bool success) {
      MidCelebration = false;
    }

    private void HandleEndCelebration(bool success) {
      MidCelebration = false;
      TargetLeft = false;
      TargetRight = false;
      if (_TiltSuccessCount >= _TiltGoalTarget) {
        RaiseMiniGameWin();
      }
    }
    /// <summary>
    /// Checks through all current robot's faces to find the closest
    /// </summary>
    /// <returns>the closest face or NULL if there are no faces to find.</returns>
    public Face ClosestFace() {
      Face closest = null;
      float closestDist = 0.0f;
      float checkDist = 0.0f;
      if (CurrentRobot.Faces.Count > 0) {
        closest = CurrentRobot.Faces[0];
        closestDist = Vector3.Distance(CurrentRobot.WorldPosition, closest.WorldPosition);
        for (int i = 0; i < CurrentRobot.Faces.Count; i++) {
          checkDist = Vector3.Distance(CurrentRobot.WorldPosition, CurrentRobot.Faces[i].WorldPosition);
          if (closestDist > checkDist) {
            closest = CurrentRobot.Faces[i];
            closestDist = checkDist;
          }
        }
      }
      return closest;
    }

    public bool IsValidFace(Face toCheck) {
      if (toCheck == null) {
        return false;
      }
      float dist = Vector3.Distance(CurrentRobot.WorldPosition, toCheck.WorldPosition);
      Debug.Log(string.Format("Current Distance : {0}", dist));
      return (dist < DistanceMax && dist > DistanceMin);
    }

    protected override void CleanUpOnDestroy() {
    }
  }
}
