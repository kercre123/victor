using UnityEngine;
using System.Collections.Generic;
using Anki.Cozmo;

namespace Cozmo.RequestGame {
  public class RequestGameManager {

    // The Last Challenge ID Cozmo has requested to play
    private ChallengeData _ChallengeToRequestData;
    public ChallengeData CurrentChallengeToRequest {
      get {
        return _ChallengeToRequestData;
      }
    }

    private IRobot _RobotToTrack = null;
    public IRobot RobotToTrack {
      get { return _RobotToTrack; }
      set {
        if (value != null) {
          TrackRobot(value);
        }
        else {
          StopTrackingRobot();
        }
      }
    }

    private ChallengeDataList ChallengeList {
      get { return ChallengeDataList.Instance; }
    }

    private BehaviorGameFlag _ChallengeToRequest = BehaviorGameFlag.NoGame;
    private float _TargetWantToPlayEmotion = 0f;
    private bool _AllowedToRequest = false;

    private void TrackRobot(IRobot robot) {
      StopTrackingRobot();

      _RobotToTrack = robot;
      _RobotToTrack.OnLightCubeAdded += HandleNumberCubesUpdated;
      _RobotToTrack.OnLightCubeRemoved += HandleNumberCubesUpdated;
    }

    private void StopTrackingRobot() {
      if (_RobotToTrack != null) {
        _RobotToTrack.OnLightCubeAdded -= HandleNumberCubesUpdated;
        _RobotToTrack.OnLightCubeRemoved -= HandleNumberCubesUpdated;
        _RobotToTrack = null;
      }
    }

    public void EnableRequestGameBehaviorGroups() {
      _AllowedToRequest = true;
      _TargetWantToPlayEmotion = CalculateWantToPlayEmotion();
      PickMiniGameToRequest();
      UpdateRobotWantToPlayState();
    }

    public void DisableRequestGameBehaviorGroups() {
      _AllowedToRequest = false;

      if (_RobotToTrack != null) {
        _RobotToTrack.SetEmotion(EmotionType.WantToPlay, 0f);
        _RobotToTrack.SetAvailableGames(BehaviorGameFlag.NoGame);
      }
    }

    public void StartRejectedRequestCooldown() {
      // TODO: Update cooldown timestamp
    }

    private void HandleNumberCubesUpdated(LightCube cube) {
      PickMiniGameToRequest();
      UpdateRobotWantToPlayState();
    }

    private void UpdateRobotWantToPlayState() {
      if (_RobotToTrack != null) {
        if (_AllowedToRequest && _TargetWantToPlayEmotion != 0f && _ChallengeToRequest != BehaviorGameFlag.NoGame) {
          _RobotToTrack.SetEmotion(EmotionType.WantToPlay, _TargetWantToPlayEmotion);
          _RobotToTrack.SetAvailableGames(_ChallengeToRequest);
        }
        else {
          _RobotToTrack.SetEmotion(EmotionType.WantToPlay, 0f);
          _RobotToTrack.SetAvailableGames(BehaviorGameFlag.NoGame);
        }
      }
    }

    private float CalculateWantToPlayEmotion() {
      float wantToPlayEmotion = 0;
      float dailyGoalProgress = (DailyGoalManager.Instance.GetTodayProgress());
      RequestGameListConfig config = RequestGameListConfig.Instance;
      float normalizedCurveValue = config.WantToPlayCurve.Evaluate(dailyGoalProgress);

      // Cozmo wants to play less as goals become more complete
      wantToPlayEmotion = Mathf.Lerp(config.WantToPlayMin, config.WantToPlayMax, normalizedCurveValue);
      return wantToPlayEmotion;
    }

    private void PickMiniGameToRequest() {
      _ChallengeToRequestData = null;
      _ChallengeToRequest = BehaviorGameFlag.NoGame;
      if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
        return;
      }
      if (_RobotToTrack == null) {
        DAS.Error("RequestGameManager.PickMiniGameToRequest", "Not tracking a robot! _RobotToTrack is NULL");
        return;
      }
      if (RequestGameListConfig.Instance == null) {
        DAS.Error("RequestGameManager.PickMiniGameToRequest", "Request minigame config is NULL");
        return;
      }
      if (ChallengeList == null) {
        DAS.Error("RequestGameManager.PickMiniGameToRequest", "Challenge List is NULL");
        return;
      }
      if (UnlockablesManager.Instance == null) {
        DAS.Error("RequestGameManager.PickMiniGameToRequest", "UnlockablesManager is NULL");
        return;
      }

      List<RequestGameConfig> requestableGames;
      List<ChallengeData> requestableGamesData;
      GetRequestableAndUnlockedGames(out requestableGames, out requestableGamesData);
      if (requestableGames.Count > 0) {
        int randomIndex = Random.Range(0, requestableGames.Count);
        RequestGameConfig randomRequestableGame = requestableGames[randomIndex];
        _ChallengeToRequest = randomRequestableGame.RequestBehaviorGameFlag;
        _ChallengeToRequestData = requestableGamesData[randomIndex];
      }
    }

    private void GetRequestableAndUnlockedGames(out List<RequestGameConfig> outRequestData, out List<ChallengeData> outChallengeData) {
      outRequestData = new List<RequestGameConfig>();
      outChallengeData = new List<ChallengeData>();
      RequestGameListConfig requestGameConfig = RequestGameListConfig.Instance;
      ChallengeData data;
      int currentNumLightCubes = _RobotToTrack.LightCubes.Count;
      for (int i = 0; i < requestGameConfig.RequestList.Length; ++i) {
        data = ChallengeList.GetChallengeDataById(requestGameConfig.RequestList[i].ChallengeID);
        if (data != null
            && data.MinigameConfig.NumCubesRequired() <= currentNumLightCubes
            && UnlockablesManager.Instance.IsUnlocked(data.UnlockId.Value)) {
          outRequestData.Add(requestGameConfig.RequestList[i]);
          outChallengeData.Add(data);
        }
      }
    }
  }
}