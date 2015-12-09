using UnityEngine;
using System.Collections;

namespace FaceTracking {
  /// <summary>
  /// Game for building face tracking skills. Has config options to enable Cozmo wandering
  /// when not paying attention to a face.
  /// </summary>
  public class FaceTrackingGame : GameBase {
    
    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();
    private int _TiltSuccessCount = 0;
    private int _TiltGoalTarget = 3;
    private float _MoveSpeed = 30.0f;

    public float TiltGoal { get; set; }

    public float ForwardSpeed { get; set; }

    public float DistanceMin { get; set; }

    public float DistanceMax { get; set; }

    public float GoalLenience { get; set; }

    public bool WanderEnabled { get; set; }

    public bool TargetLeft { get; set; }

    public bool MidCelebration { get; set; }

    [SerializeField]
    private FaceTrackingGamePanel _GamePanelPrefab;
    private FaceTrackingGamePanel _GamePanel;

    protected override void Initialize(MinigameConfigBase minigameConfigData) {
      FaceTrackingGameConfig config = (minigameConfigData as FaceTrackingGameConfig);
      _TiltGoalTarget = config.Goal;
      _MoveSpeed = config.MoveSpeed;
      WanderEnabled = config.WanderEnabled;
      TiltGoal = config.TiltTreshold;
      GoalLenience = config.Lenience;
      TargetLeft = true;
      MidCelebration = false;
      InitializeMinigameObjects();
    }

    // Calculates tilt based on the current face, triggers success if you've gotten to the right threshold
    public float CalculateTilt(Face face) {
      float angle = Vector3.Cross(CurrentRobot.Forward, face.WorldPosition - CurrentRobot.WorldPosition).z;
      // If the Angle is (< 0). Face is to your right, if (> 0) Face is to your left.
      float tiltVal = angle / TiltGoal;
      if (!TargetLeft) {
        tiltVal *= -1.0f;
      }

      // Determine success, or if we're even headed in the right direction, don't trigger success
      // if we are too far to the desired direction
      if (1.0f >= tiltVal && tiltVal >= (1.0f-GoalLenience) && !MidCelebration) {
        TiltSuccess();
      }
      else if (1.0f > tiltVal && tiltVal > 0.0) {
        // Set light to Green if heading in the right direction but not there yet.
        CurrentRobot.SetBackpackBarLED(Anki.Cozmo.LEDId.LED_BACKPACK_MIDDLE, Color.green);
        _GamePanel.SetProgressBar(tiltVal);
      }
      else {
        // Set light to Red if heading in the wrong direction
        CurrentRobot.SetBackpackBarLED(Anki.Cozmo.LEDId.LED_BACKPACK_MIDDLE, Color.red);
      }

      return tiltVal;
    }

    public void TiltSuccess() {
      _TiltSuccessCount++;
      _GamePanel.SetPoints(_TiltSuccessCount);
      MidCelebration = true;
      CurrentRobot.SendAnimation(AnimationName.kFinishTapCubeWin,HandleEndCelebration);
    }

    protected void InitializeMinigameObjects() {
      
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("PeekGameStateMachine", _StateMachine);

      _GamePanel = UIManager.OpenView(_GamePanelPrefab).GetComponent<FaceTrackingGamePanel>();
      _GamePanel.SetPoints(_TiltSuccessCount);
      _GamePanel.SetArrowFacing(TargetLeft);

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
      SetSpeed();
    }

    void SetSpeed() {
      ForwardSpeed = _MoveSpeed;
      DistanceMax = 800.0f;
      DistanceMin = 400.0f;
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    private void HandleEndCelebration(bool success) {
      MidCelebration = false;
      TargetLeft = !TargetLeft;
      _GamePanel.SetArrowFacing(TargetLeft);
      if (_TiltSuccessCount >= _TiltGoalTarget) {
        RaiseMiniGameWin();
      }
    }

    protected override void CleanUpOnDestroy() {
      CurrentRobot.ActivateBehaviorChooser(Anki.Cozmo.BehaviorChooserType.Selection);
      CurrentRobot.ExecuteBehavior(Anki.Cozmo.BehaviorType.NoneBehavior);
      if (_GamePanel != null) {
        UIManager.CloseViewImmediately(_GamePanel);
      }
    }
  }
}
