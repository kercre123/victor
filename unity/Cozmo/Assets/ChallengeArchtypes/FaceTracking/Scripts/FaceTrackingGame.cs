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
    private float _TiltGoalThreshold = 30.0f; 
    private float _MoveSpeed = 30.0f;

    public float TiltGoal { get;}

    public float ForwardSpeed { get; set; }

    public float DistanceMin { get; set; }

    public float DistanceMax { get; set; }

    public bool WanderEnabled { get; set; }

    public bool TargetLeft { get; set; }

    [SerializeField]
    private FaceTrackingGamePanel _GamePanelPrefab;
    private FaceTrackingGamePanel _GamePanel;

    protected override void Initialize(MinigameConfigBase minigameConfigData) {
      FaceTrackingGameConfig config = (minigameConfigData as FaceTrackingGameConfig);
      _TiltGoalTarget = config.Goal;
      _MoveSpeed = config.MoveSpeed;
      WanderEnabled = config.WanderEnabled;
      _TiltGoalThreshold = config.TiltTreshold;

      InitializeMinigameObjects();
    }

    // Returns true if % tilt matches target treshold
    public void TiltSuccessCheck(Face face) {
      float angle = Vector3.Cross(CurrentRobot.Forward, face.WorldPosition - CurrentRobot.WorldPosition).z;
      // If the Angle is (< 0). Face is to your right, if (> 0) Face is to your left.
      float tiltVal = Mathf.Abs(angle / _TiltGoalThreshold);
      Debug.Log(string.Format("RYAN -- Angle : {0} -- Goal : {2} -- TiltVal : {1} ",angle,tiltVal,_TiltGoalThreshold));

      // TODO : Set up Cozmo's face to reflect current status/progress
      if (tiltVal >= 1.0f) {
        TiltSuccess();
      }
      else if (tiltVal > 0.0) {
        // Set light to Green if heading in the right direction but not there yet.
        CurrentRobot.SetBackpackBarLED(Anki.Cozmo.LEDId.LED_BACKPACK_MIDDLE, Color.green);
      }

    }

    public void TiltSuccess() {
      _TiltSuccessCount++;
      _GamePanel.SetPoints(_TiltSuccessCount);
      CurrentRobot.SendAnimation(AnimationName.kHappyA);
      TargetLeft = !TargetLeft;
      if (_TiltSuccessCount >= _TiltGoalTarget) {
        RaiseMiniGameWin();
      }
    }

    protected void InitializeMinigameObjects() {
      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("PeekGameStateMachine", _StateMachine);
      _GamePanel = UIManager.OpenView(_GamePanelPrefab).GetComponent<FaceTrackingGamePanel>();
      _GamePanel.SetPoints(_TiltSuccessCount);

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
      TargetLeft = true;
    }

    void SetSpeed() {
      ForwardSpeed = _MoveSpeed;
      DistanceMax = 200.0f;
      DistanceMin = 100.0f;
    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
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
