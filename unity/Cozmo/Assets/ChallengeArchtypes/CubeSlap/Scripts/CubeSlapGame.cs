using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using AnimationGroups;

namespace CubeSlap {
  
  public class CubeSlapGame : GameBase {

    public const string kSetUp = "SetUp";
    public const string kWaitForPounce = "WaitForPounce";
    public const string kCozmoWinEarly = "CozmoWinEarly";
    public const string kCozmoWinPounce = "CozmoWinPounce";
    public const string kPlayerWin = "PlayerWin";
    // Consts for determining the exact placement and forgiveness for cube location
    // Must be consistent for animations to work
    public const float kCubePlaceDist = 80.0f;
    public const float kCubeLostDelay = 0.25f;

    private float _MinSlapDelay;
    private float _MaxSlapDelay;
    private int _SuccessGoal;
    private int _SuccessCount;
    private bool _CliffFlagTrown = false;
    // Flag to keep track if we've actually done the Pounce animation this round
    private bool _SlapFlagThrown = false;
    private float _CurrentSlapChance;
    private float _BaseSlapChance;
    private int _MaxFakeouts;

    private int _SlapCount;

    private LightCube _CurrentTarget = null;

    [SerializeField]
    private List<string> _FakeoutAnimations;
    [SerializeField]
    private string _PounceAnimation;
    [SerializeField]
    private string _RetractAnimation;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      CubeSlapConfig config = minigameConfig as CubeSlapConfig;
      MaxAttempts = config.MaxAttempts;
      _SuccessGoal = config.SuccessGoal;
      _MinSlapDelay = config.MinSlapDelay;
      _MaxSlapDelay = config.MaxSlapDelay;
      _BaseSlapChance = config.StartingSlapChance;
      _MaxFakeouts = config.MaxFakeouts;
      _SuccessCount = 0;
      NumSegments = _SuccessGoal;
      Progress = 0.0f;
      InitializeMinigameObjects(config.NumCubesRequired());
    }

    protected void InitializeMinigameObjects(int numCubes) {

      CurrentRobot.SetBehaviorSystem(false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetHeadAngle(-1.0f);
      CurrentRobot.SetLiftHeight(0.0f);

      RobotEngineManager.Instance.OnCliffEvent += HandleCliffEvent;

      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new SeekState(), numCubes, InitialCubesDone);
      _StateMachine.SetNextState(initCubeState);
    }

    private void InitialCubesDone() {
      _CurrentTarget = GetClosestAvailableBlock();
    }

    protected override void CleanUpOnDestroy() {

      RobotEngineManager.Instance.OnCliffEvent -= HandleCliffEvent;
      
    }

    public LightCube GetCurrentTarget() {
      if (_CurrentTarget == null) {
        _CurrentTarget = GetClosestAvailableBlock();
      }
      return _CurrentTarget;
    }

    private LightCube GetClosestAvailableBlock() {
      float minDist = float.MaxValue;
      ObservedObject closest = null;

      for (int i = 0; i < CurrentRobot.SeenObjects.Count; ++i) {
        if (CurrentRobot.SeenObjects[i] is LightCube) {
          float d = Vector3.Distance(CurrentRobot.SeenObjects[i].WorldPosition, CurrentRobot.WorldPosition);
          if (d < minDist) {
            minDist = d;
            closest = CurrentRobot.SeenObjects[i];
          }
        }
      }
      return closest as LightCube;
    }

    // Attempt the pounce
    public void AttemptSlap() {
      float SlapRoll;
      if (_MaxFakeouts <= 0) {
        SlapRoll = 0.0f;
      }
      else {
        SlapRoll = Random.Range(0.0f, 1.0f);
      }
      if (SlapRoll <= _CurrentSlapChance) {
        // Enter Animation State to attempt a pounce.
        // Set Callback for HandleEndSlapAttempt
        _CliffFlagTrown = false;
        _SlapFlagThrown = true;
        CurrentRobot.SendAnimation(_PounceAnimation, HandleEndSlapAttempt);
      }
      else {
        // If you do a fakeout instead, increase the likelyhood of a slap
        // attempt based on the max number of fakeouts.
        int rand = Random.Range(0, _FakeoutAnimations.Count);
        _CurrentSlapChance += ((1.0f - _BaseSlapChance) / _MaxFakeouts);
        _StateMachine.SetNextState(new AnimationState(_FakeoutAnimations[rand], HandleFakeoutEnd));
      }
    }

    private void HandleEndSlapAttempt(bool success) {
      // If the animation completes and the cube is beneath Cozmo,
      // Cozmo has won.
      if (_CliffFlagTrown) {
        ShowHowToPlaySlide(kCozmoWinPounce);
        OnFailure();
        return;
      }
      else {
        // If the animation completes Cozmo is not on top of the Cube,
        // The player has won this round
        ShowHowToPlaySlide(kPlayerWin);
        OnSuccess();
      }
    }

    public void HandleFakeoutEnd(bool success) {
      _StateMachine.SetNextState(new SlapGameState());
    }

    private void HandleCliffEvent(Anki.Cozmo.CliffEvent cliff) {
      // Ignore if it throws this without a cliff actually detected
      if (!cliff.detected) {
        return;
      }
      _CliffFlagTrown = true;
    }

    public void OnSuccess() {
      _SuccessCount++;
      Progress = ((float)_SuccessCount / (float)_SuccessGoal);
      _StateMachine.SetNextState(new AnimationState(AnimationName.kMajorFail, HandleAnimationDone));
    }

    public void OnFailure() {
      TryDecrementAttempts();
      _StateMachine.SetNextState(new AnimationGroupState(AnimationGroupName.kWin, HandleAnimationDone));
    }

    public void HandleAnimationDone(bool success) {
      // Determines winner and loser at the end of Cozmo's animation, or returns
      // to seek state for the next round
      if (_SuccessCount >= _SuccessGoal) {
        RaiseMiniGameWin();
      }
      else if (AttemptsLeft <= 0) {
        RaiseMiniGameLose();
      }
      else if (_SlapFlagThrown) {
        _SlapFlagThrown = false;
        _StateMachine.SetNextState(new AnimationState(_RetractAnimation, HandleRetractDone));
      }
      else {
        _StateMachine.SetNextState(new SeekState());
      }
    }

    public void HandleRetractDone(bool success) {
      _StateMachine.SetNextState(new SeekState());
    }

    public float GetSlapDelay() {
      return Random.Range(_MinSlapDelay, _MaxSlapDelay);
    }

    public void ResetSlapChance() {
      _CurrentSlapChance = _BaseSlapChance;
    }

    protected override int CalculateExcitementStatRewards() {
      return (MaxAttempts - AttemptsLeft);
    }
 
  }
}
