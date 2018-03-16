using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Cozmo.Challenge.CubePounce {
  // TODO : SCORE RELATED LOGIC HAS BEEN MOVED TO THE SCORING REGION OF GAMEBASE, private fields for score/rounds are obsolete
  public class CubePounceGame : GameBase {

    public enum Zone {
      MousetrapClose,   // If Cozmo sees a cube in this zone he tries to pounce immediately
      Pounceable,       // In this zone Cozmo will regularly try to fake out/pounce on cube
      InPlay            // In this zone Cozmo will have his lift raised and the cube light will be green
    }

    public enum DistanceType {
      Tight,
      Loose,
      Exact
    }

    // Consts for determining the exact placement and forgiveness for cube location
    // Animators have been measuring distance from faceplate to cube; the below const is the distance
    // between the faceplate and front weel center plane
    private const float _kWheelCenterToFacePlane_mm = 13.0f;

    private float _CubeTargetSeenTime_s = -1;

    private bool _CubeSeenRecently = false;

    private bool _CubeCurrentlyMoving = false;

    private bool _CubeReadyForCreep = false;
    private Vector3 _CubeUnmovedPosition = new Vector3();
    private float _CubeLastMovedTime_s = 0.0f;

    public bool CurrentlyInFakeoutState { get; set; }

    public bool CubeSeenRecently { get { return _CubeSeenRecently; } }

    public bool CubeReadyForCreep { get { return _CubeReadyForCreep; } }

    public CubePounceConfig GameConfig;

    private float _CurrentPounceChance;
    private LightCube _CurrentTarget = null;

    private Coroutine _CheckMovePenaltyCoroutine;

    public bool AllRoundsCompleted {
      get {
        int winningScore = Mathf.Max(HumanRoundsWon, CozmoRoundsWon);
        return (2 * winningScore > TotalRounds);
      }
    }

    protected override void InitializeGame(ChallengeConfigBase challengeConfigData) {
      GameConfig = challengeConfigData as CubePounceConfig;
      TotalRounds = GameConfig.Rounds;
      MaxScorePerRound = GameConfig.MaxScorePerRound;
      _CurrentTarget = null;
      InitializeChallengeObjects(GameConfig.NumCubesRequired());
      LightCube.OnMovedAction += HandleCubeMoved;
      LightCube.OnStoppedAction += HandleCubeStopped;
      _ShowEndWinnerSlide = true;
      SharedMinigameView.CurrentScoreBoardType = MinigameWidgets.SharedMinigameView.ScoreBoardType.Slim;
    }

    protected void InitializeChallengeObjects(int numCubes) {
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingFaces, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingPets, false);
      CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.DetectingOverheadEdges, false);

      _StateMachine.SetNextState(new InitialCubesState(new CubePounceStateInitGame(), numCubes));
    }

    protected override void CleanUpOnDestroy() {
      if (null != CurrentRobot) {
        CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceGetOut, null);
      }
      LightCube.OnMovedAction -= HandleCubeMoved;
      LightCube.OnStoppedAction -= HandleCubeStopped;
      if (null != _CheckMovePenaltyCoroutine) {
        StopCoroutine(_CheckMovePenaltyCoroutine);
      }
    }

    public LightCube GetCubeTarget() {
      return _CurrentTarget;
    }

    public void SelectCubeTarget() {
      if (this.CubeIdsForGame.Count > 0) {
        _CurrentTarget = CurrentRobot.LightCubes[this.CubeIdsForGame[0]];
      }
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

    public bool WithinDistance(Zone zone, DistanceType distType) {
      float checkDistance = GetDistanceForZone(zone);

      switch (distType) {
      case DistanceType.Tight:
        checkDistance -= GetSlushDistanceForZone(zone) * 0.5f;
        break;
      case DistanceType.Loose:
        checkDistance += GetSlushDistanceForZone(zone) * 0.5f;
        break;
      }

      return WithinPounceDistance(checkDistance);
    }

    private float GetDistanceForZone(Zone zone) {
      float distance = _kWheelCenterToFacePlane_mm;

      switch (zone) {
      case Zone.MousetrapClose:
        distance += GameConfig.CubeDistanceTooClose_mm;
        break;
      case Zone.Pounceable:
        distance += GameConfig.CubeDistanceBetween_mm;
        break;
      case Zone.InPlay:
        distance += GameConfig.CubeDistanceBetween_mm + GameConfig.CubeDistanceGreyZone_mm;
        break;
      }

      return distance;
    }

    private float GetSlushDistanceForZone(Zone zone) {
      switch (zone) {
      case Zone.InPlay:
        return 20.0f;
      case Zone.Pounceable:
        return 5.0f;
      default:
        return 0.0f;
      }
    }

    public float GetNextCreepDistance() {
      float nextCreepDistance_mm = Random.Range(GameConfig.CreepDistanceMin_mm, GameConfig.CreepDistanceMax_mm);
      float actualDistanceSqr_mm = 0.0f;
      const float distanceOverreach_mm = 3.0f;
      float closestDistanceAllowed_mm = GetDistanceForZone(Zone.Pounceable) - distanceOverreach_mm;
      if (CozmoUtil.ObjectEdgeWithinXYDistance(CurrentRobot.WorldPosition, GetCubeTarget(), nextCreepDistance_mm + closestDistanceAllowed_mm, out actualDistanceSqr_mm)) {
        nextCreepDistance_mm = Mathf.Sqrt(actualDistanceSqr_mm) - closestDistanceAllowed_mm;
        if (nextCreepDistance_mm < 0.0f) {
          nextCreepDistance_mm = 0.0f;
        }
      }

      return nextCreepDistance_mm;
    }

    // Returns whether we just finished a round
    public bool CheckAndUpdateRoundScore() {
      // If we haven't yet hit our max score nothing to see here
      if (Mathf.Max(CozmoScore, HumanScore) < GameConfig.MaxScorePerRound) {
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

        if (CubeHasMovedSignificantly(targetCube.WorldPosition)) {
          _CubeUnmovedPosition = targetCube.WorldPosition;
          _CubeReadyForCreep = false;
          _CubeLastMovedTime_s = Time.time;
        }
        else if ((Time.time - _CubeLastMovedTime_s) > GameConfig.CreepMinUnmovedTime_s) {
          _CubeReadyForCreep = true;
        }
      }
      // If it's been too long our cube isn't visible anymore
      else if ((currentTime_s - _CubeTargetSeenTime_s) > GameConfig.CubeVisibleBufferTime_s) {
        _CubeSeenRecently = false;
      }
    }

    private bool CubeHasMovedSignificantly(Vector3 currentPosition) {
      Vector3 posDiff = currentPosition - _CubeUnmovedPosition;
      posDiff.z = 0.0f;

      float diffMagSqr = posDiff.sqrMagnitude;
      float threshMagSqr = GameConfig.CubeMovedThresholdDistance_mm * GameConfig.CubeMovedThresholdDistance_mm;

      return diffMagSqr > threshMagSqr;
    }

    public void UpdateScoreboard() {
      int halfTotalRounds = (TotalRounds + 1) / 2;
      Cozmo.MinigameWidgets.ScoreWidget cozmoScoreWidget = SharedMinigameView.CozmoScoreboard;
      cozmoScoreWidget.Score = CozmoScore;
      cozmoScoreWidget.MaxRounds = halfTotalRounds;
      cozmoScoreWidget.RoundsWon = CozmoRoundsWon;

      Cozmo.MinigameWidgets.ScoreWidget playerScoreWidget = SharedMinigameView.PlayerScoreboard;
      playerScoreWidget.Score = HumanScore;
      playerScoreWidget.MaxRounds = halfTotalRounds;
      playerScoreWidget.RoundsWon = HumanRoundsWon;
    }

    protected override void SendCustomEndGameDasEvents() {
      Dictionary<string, string> quitGameScoreKeyValues = new Dictionary<string, string>();
      Dictionary<string, string> quitGameRoundsWonKeyValues = new Dictionary<string, string>();

      quitGameScoreKeyValues.Add("CozmoScore", CozmoScore.ToString());
      quitGameRoundsWonKeyValues.Add("CozmoRoundsWon", CozmoRoundsWon.ToString());

      DAS.Event(DASConstants.Game.kQuitGameScore, HumanScore.ToString(), quitGameScoreKeyValues);
      DAS.Event(DASConstants.Game.kQuitGameRoundsWon, HumanRoundsWon.ToString(), quitGameRoundsWonKeyValues);
    }

    private void HandleCubeMoved(int id, float accX, float accY, float aaZ) {
      if (null != _CurrentTarget && id == _CurrentTarget.ID) {
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
      if (null != _CurrentTarget && id == _CurrentTarget.ID) {
        _CubeCurrentlyMoving = false;
      }
    }

    public bool PitchIndicatesPounceSuccess(float pitchToCheck_deg) {
      float currentPitch_deg = Mathf.Rad2Deg * CurrentRobot.PitchAngle;
      float angleChange_deg = Mathf.Abs(Mathf.DeltaAngle(pitchToCheck_deg, currentPitch_deg));
      if (angleChange_deg > GameConfig.PouncePitchDiffSuccess_deg) {
        return true;
      }
      return false;
    }

    protected override void InitializeReactionaryBehaviorsForGameStart() {
      RobotEngineManager.Instance.CurrentRobot.DisableReactionsWithLock(ReactionaryBehaviorEnableGroups.kMinigameId, ReactionaryBehaviorEnableGroups.kCubePounceGameTriggers);
    }

    protected override void ShowWinnerState(int currentEndIndex, string overrideWinnerText = null, string footerText = "", bool showWinnerTextInShelf = false) {
      footerText = Localization.GetWithArgs(LocalizationKeys.kSpeedTapTextRoundScore, HumanScore, CozmoScore);
      SharedMinigameView.HideCozmoScoreboard();
      SharedMinigameView.HidePlayerScoreboard();
      base.ShowWinnerState(currentEndIndex, overrideWinnerText, footerText, showWinnerTextInShelf);
    }
  }
}
