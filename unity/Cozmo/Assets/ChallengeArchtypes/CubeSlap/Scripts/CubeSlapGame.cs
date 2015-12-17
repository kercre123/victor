using UnityEngine;
using System.Collections;

namespace CubeSlap {
  
  public class CubeSlapGame : GameBase {

    public const string kSetUp = "SetUp";
    public const string kWaitForPounce = "WaitForPounce";
    public const string kCozmoWinEarly = "CozmoWinEarly";
    public const string kCozmoWinPounce = "CozmoWinPounce";
    public const string kPlayerWin = "PlayerWin";

    private float _MinSlapDelay;
    private float _MaxSlapDelay;
    private int _SuccessGoal;
    private int _SuccessCount;
    private bool _CliffFlagTrown = false;
    private float _SlapChance;
    private float _BaseSlapChance;
    private int _MaxFakeouts;

    private LightCube _CurrentTarget = null;

    protected override void Initialize(MinigameConfigBase minigameConfig) {
      CubeSlapConfig config = minigameConfig as CubeSlapConfig;
      MaxAttempts = config.MaxAttempts;
      _SuccessGoal = config.SuccessGoal;
      _MinSlapDelay = config.MinSlapDelay;
      _MaxSlapDelay = config.MaxSlapDelay;
      _BaseSlapChance = config.StartingSlapChance;
      _MaxFakeouts = config.MaxFakeouts;
      _SuccessCount = 0;
      Progress = 0.0f;
      InitializeMinigameObjects();
    }

    protected void InitializeMinigameObjects() {

      CurrentRobot.SetBehaviorSystem(false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetHeadAngle(-1.0f);
      CurrentRobot.SetLiftHeight(0.0f);

      RobotEngineManager.Instance.OnCliffEvent += HandleCliffEvent;

      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new SeekState(), 1, true, InitialCubesDone);
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
        SlapRoll = Random.Range(0.0f,1.0f);
      }
      if (SlapRoll <= _SlapChance) {
        Debug.Log("CubeSlap - Attempt Slap now");
        // Enter Animation State to attempt a pounce.
        // Set Callback for HandleEndSlapAttempt
        _CliffFlagTrown = false;
        CurrentRobot.SendAnimation(AnimationName.kPounceForward, HandleEndSlapAttempt);
      }
      else {
        // If you do a fakeout instead, increase the likelyhood of a slap
        // attempt based on the max number of fakeouts.
        int rand = Random.Range(1, 4);
        Debug.Log(string.Format("CubeSlap - Attempt pounceFakeout{0}",rand));
        _SlapChance += ((1.0f - _BaseSlapChance) / _MaxFakeouts);
        AnimationState animState = new AnimationState();
        animState.Initialize(string.Format("pounceFakeout{0}",rand), HandleFakeoutEnd);
        _StateMachine.SetNextState(animState);
      }
    }

    private void HandleEndSlapAttempt(bool success) {
      // If the animation completes and the cube is beneath Cozmo,
      // Cozmo has won.
      Debug.Log("CubeSlap - Slap Ended");
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
      Debug.Log("CubeSlap - Handle Cliff Event");
      if (!cliff.detected) {
        return;
      }
      Debug.Log("CubeSlap - Handle Cliff Event - Cliff Detected");
      _CliffFlagTrown = true;
    }

    public void OnSuccess() {
      _SuccessCount++;
      Debug.Log("CubeSlap - Player Wins");
      Progress = _SuccessCount / _SuccessGoal;
      AnimationState animState = new AnimationState();
      animState.Initialize("VeryIrritated", HandleAnimationDone);
      _StateMachine.SetNextState(animState);
    }

    public void OnFailure() {
      Debug.Log("CubeSlap - Cozmo Wins");
      TryDecrementAttempts();
      AnimationState animState = new AnimationState();
      animState.Initialize(AnimationName.kMajorWin, HandleAnimationDone);
      _StateMachine.SetNextState(animState);
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
      else {
        _StateMachine.SetNextState(new SeekState());
      }
    }

    public float GetSlapDelay() {
      return Random.Range(_MinSlapDelay,_MaxSlapDelay);
    }

    public void ResetSlapChance() {
      _SlapChance = _BaseSlapChance;
    }
 
  }
}
