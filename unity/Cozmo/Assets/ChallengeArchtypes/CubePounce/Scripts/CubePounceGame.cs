using UnityEngine;
using Anki.Cozmo.Audio;
using System.Collections;
using System.Collections.Generic;
using System.Threading;

namespace Cozmo.Minigame.CubePounce {
  // TODO : SCORE RELATED LOGIC HAS BEEN MOVED TO THE SCORING REGION OF GAMEBASE, private fields for score/rounds are obsolete
  public class CubePounceGame : GameBase {

    // Consts for determining the exact placement and forgiveness for cube location
    // Animators have been measuring distance from faceplate to cube; the below const is the distance
    // between the faceplate and front weel center plane
    private const float _kWheelCenterToFacePlane_mm = 13.0f;

    public float CubePlaceDistTight_mm { get { return GameConfig.CubeDistanceBetween_mm + _kWheelCenterToFacePlane_mm; } }

    public float CubePlaceDistLoose_mm { get { return GameConfig.CubeDistanceBetween_mm + _kWheelCenterToFacePlane_mm + GameConfig.CubeDistanceGreyZone_mm; } }

    public float CubePlaceDistTooClose_mm { get { return GameConfig.CubeDistanceTooClose_mm + _kWheelCenterToFacePlane_mm; } }

    private float _CubeTargetSeenTime_s = -1;

    private bool _CubeSeenRecently = false;

    private bool _CubeCurrentlyMoving = false;

    public bool CurrentlyInFakeoutState { get; set; }

    public bool CubeSeenRecently { get { return _CubeSeenRecently; } }

    public CubePounceConfig GameConfig;

    private float _CurrentPounceChance;
    private LightCube _CurrentTarget = null;

    private Coroutine _CheckMovePenaltyCoroutine;

    public bool AllRoundsCompleted {
      get {
        int winningScore = Mathf.Max(PlayerRoundsWon, CozmoRoundsWon);
        return (2 * winningScore > TotalRounds);
      }
    }

    protected override void InitializeGame(MinigameConfigBase minigameConfig) {
      GameConfig = minigameConfig as CubePounceConfig;
      TotalRounds = GameConfig.Rounds;
      MaxScorePerRound = GameConfig.MaxScorePerRound;
      CozmoScore = 0;
      PlayerScore = 0;
      PlayerRoundsWon = 0;
      CozmoRoundsWon = 0;
      _CurrentTarget = null;
      InitializeMinigameObjects(GameConfig.NumCubesRequired());
      CurrentRobot.SetEnableCliffSensor(false);
      LightCube.OnMovedAction += HandleCubeMoved;
      LightCube.OnStoppedAction += HandleCubeStopped;
    }

    protected void InitializeMinigameObjects(int numCubes) {

      CurrentRobot.SetEnableFreeplayBehaviorChooser(false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);

      _StateMachine.SetNextState(new InitialCubesState(new CubePounceStateInitGame(), numCubes));
    }

    protected override void CleanUpOnDestroy() {
      if (null != CurrentRobot) {
        CurrentRobot.SetEnableCliffSensor(true);
        CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceGetOut, null);
        CurrentRobot.SetIdleAnimation(Anki.Cozmo.AnimationTrigger.Count);
      }
      LightCube.OnMovedAction -= HandleCubeMoved;
      LightCube.OnStoppedAction -= HandleCubeStopped;
      if (null != _CheckMovePenaltyCoroutine) {
        StopCoroutine(_CheckMovePenaltyCoroutine);
      }
    }

    public LightCube GetCubeTarget() {
      if (_CurrentTarget == null) {
        if (this.CubeIdsForGame.Count > 0) {
          _CurrentTarget = CurrentRobot.LightCubes[this.CubeIdsForGame[0]];
        }
      }
      return _CurrentTarget;
    }

    public void ResetPounceChance() {
      DAS.Debug("CubePounceGame.ResetPounceChance", "");
      _CurrentPounceChance = GameConfig.BasePounceChance;
    }

    public void IncreasePounceChance() {
      if (_CurrentPounceChance < 1.0f) {
        _CurrentPounceChance += GameConfig.PounceChanceIncrement;
        _CurrentPounceChance = Mathf.Clamp(_CurrentPounceChance, 0.0f, 1.0f);
      }
    }

    public State GetNextFakeoutOrAttemptState() {
      DAS.Debug("CubePounceGame.SetNextFakeoutOrAttemptState", "Pounce chance: " + _CurrentPounceChance);
      bool shouldPounce = (Random.Range(0.0f, 1.0f) <= _CurrentPounceChance);

      if (shouldPounce) {
        return new CubePounceStateAttempt();
      }
      else {
        return new CubePounceStateFakeOut();
      }
    }

    public bool WithinPounceDistance(float distanceThreshold_mm) {
      return CozmoUtil.ObjectEdgeWithinXYDistance(CurrentRobot.WorldPosition, GetCubeTarget(), distanceThreshold_mm);
    }

    public bool WithinAngleTolerance() {
      return CozmoUtil.PointWithinXYAngleTolerance(CurrentRobot.WorldPosition, GetCubeTarget().WorldPosition, CurrentRobot.Rotation.eulerAngles.z, GameConfig.CubeFacingAngleTolerance_deg);
    }

    // Returns whether we just finished a round
    public bool CheckAndUpdateRoundScore() {
      // If we haven't yet hit our max score nothing to see here
      if (Mathf.Max(CozmoScore, PlayerScore) < GameConfig.MaxScorePerRound) {
        return false;
      }
      EndCurrentRound();
      return true;
    }

    public float GetAttemptDelay() {
      return Random.Range(GameConfig.MinAttemptDelay_s, GameConfig.MaxAttemptDelay_s);
    }

    public void UpdateCubeVisibility() {
      float currentTime_s = Time.time;
      LightCube targetCube = GetCubeTarget();
      if (null != targetCube && targetCube.IsInFieldOfView) {
        _CubeTargetSeenTime_s = currentTime_s;
        _CubeSeenRecently = true;
      }
      // If it's been too long our cube isn't visible anymore
      else if ((currentTime_s - _CubeTargetSeenTime_s) > GameConfig.CubeVisibleBufferTime_s) {
        _CubeSeenRecently = false;
      }
    }

    public void UpdateScoreboard() {
      int halfTotalRounds = (TotalRounds + 1) / 2;
      Cozmo.MinigameWidgets.ScoreWidget cozmoScoreWidget = SharedMinigameView.CozmoScoreboard;
      cozmoScoreWidget.Score = CozmoScore;
      cozmoScoreWidget.MaxRounds = halfTotalRounds;
      cozmoScoreWidget.RoundsWon = CozmoRoundsWon;

      Cozmo.MinigameWidgets.ScoreWidget playerScoreWidget = SharedMinigameView.PlayerScoreboard;
      playerScoreWidget.Score = PlayerScore;
      playerScoreWidget.MaxRounds = halfTotalRounds;
      playerScoreWidget.RoundsWon = PlayerRoundsWon;

      if (AllRoundsCompleted) {
        // Hide Current Round at end
        SharedMinigameView.InfoTitleText = string.Empty;
      }
      else {
        // Display the current round
        SharedMinigameView.InfoTitleText = Localization.GetWithArgs(LocalizationKeys.kSpeedTapRoundsText, CozmoRoundsWon + PlayerRoundsWon + 1);
      }
    }

    protected override void SendCustomEndGameDasEvents() {
      Dictionary<string, string> quitGameScoreKeyValues = new Dictionary<string, string>();
      Dictionary<string, string> quitGameRoundsWonKeyValues = new Dictionary<string, string>();

      quitGameScoreKeyValues.Add("CozmoScore", CozmoScore.ToString());
      quitGameRoundsWonKeyValues.Add("CozmoRoundsWon", CozmoRoundsWon.ToString());

      DAS.Event(DASConstants.Game.kQuitGameScore, PlayerScore.ToString(), null, quitGameScoreKeyValues);
      DAS.Event(DASConstants.Game.kQuitGameRoundsWon, PlayerRoundsWon.ToString(), null, quitGameRoundsWonKeyValues);
    }

    private void HandleCubeMoved(int id, float accX, float accY, float aaZ) {
      if (id == GetCubeTarget().ID) {
        _CubeCurrentlyMoving = true;

        if (CurrentlyInFakeoutState && GameConfig.FakeoutMovePenaltyEnabled) {
          // Set this to false immediately so we don't get multiple penalties during the same fakeout
          CurrentlyInFakeoutState = false;

          // Start a routine that will check whether the cube is moving and grant a point if needed
          _CheckMovePenaltyCoroutine = StartCoroutine(CheckMovePenalty());
        }
      }
    }

    private IEnumerator CheckMovePenalty() {
      yield return new WaitForSeconds(GameConfig.FakeoutCubeMoveTime_s);

      if (_CubeCurrentlyMoving) { 
        _StateMachine.SetNextState(new CubePounceStatePostPoint(cozmoWon: true));
      }
    }

    private void HandleCubeStopped(int id) {
      if (id == GetCubeTarget().ID) {
        _CubeCurrentlyMoving = false;
      }
    }
  }
}
