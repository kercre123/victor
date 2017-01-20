using UnityEngine;
using System.Collections.Generic;
using Anki.Cozmo;
using Anki.Debug;

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
    private bool _AllowedToRequest = false;

    private bool _IsDebugRequest = false;

    private void TrackRobot(IRobot robot) {
      StopTrackingRobot();

      _RobotToTrack = robot;
      _RobotToTrack.OnLightCubeAdded += HandleNumberCubesUpdated;
      _RobotToTrack.OnLightCubeRemoved += HandleNumberCubesUpdated;

      DebugConsoleData.Instance.AddConsoleFunction("Select Game To Request", "Game Request", DebugSelectGameToRequest);
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
      // Update cooldown timestamp
      if (RequestGameListConfig.Instance != null) {
        RequestGameListConfig requestGameConfig = RequestGameListConfig.Instance;
        for (int i = 0; i < requestGameConfig.RequestList.Length; ++i) {
          if (_ChallengeToRequest == requestGameConfig.RequestList[i].RequestBehaviorGameFlag) {
            requestGameConfig.RequestList[i].RejectGame();
            break;
          }
        }
      }
    }

    public void StartGameRequested() {
      // Mark always once the popup shows up, after that should show the others
      if (RequestGameListConfig.Instance != null) {
        RequestGameListConfig requestGameConfig = RequestGameListConfig.Instance;
        for (int i = 0; i < requestGameConfig.RequestList.Length; ++i) {
          requestGameConfig.RequestList[i].WasSelectedPrevious = (_ChallengeToRequest == requestGameConfig.RequestList[i].RequestBehaviorGameFlag);
        }
      }
    }

    private void HandleNumberCubesUpdated(LightCube cube) {
      PickMiniGameToRequest();
      UpdateRobotWantToPlayState();
    }

    private void UpdateRobotWantToPlayState() {
      if (_RobotToTrack != null) {
        if (_AllowedToRequest && _ChallengeToRequest != BehaviorGameFlag.NoGame) {
          _RobotToTrack.SetAvailableGames(_ChallengeToRequest);
          // Want to play is actually binary and not really an emotion.
          // it still needs to be set high to enable.
          // Game request timing is also mainly based on the goal.
          _RobotToTrack.SetEmotion(EmotionType.WantToPlay, 1f);
        }
        else {
          _RobotToTrack.SetEmotion(EmotionType.WantToPlay, 0f);
          _RobotToTrack.SetAvailableGames(BehaviorGameFlag.NoGame);
        }
      }
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
      List<float> requestableGamesScores;
      GetRequestableAndUnlockedGames(out requestableGames, out requestableGamesData, out requestableGamesScores);
      // Scores can be weighting for Daily goals, or have different base values
      if (requestableGames.Count > 0) {
        float weightedTotal = 0;
        for (int i = 0; i < requestableGames.Count; ++i) {
          weightedTotal += requestableGamesScores[i];
        }
        float roll = Random.Range(0.0f, weightedTotal);
        int randomIndex = 0;
        for (int i = 0; i < requestableGamesScores.Count; i++) {
          if (roll <= requestableGamesScores[i]) {
            randomIndex = i;
            break;
          }
          roll -= requestableGamesScores[i];
        }

        RequestGameConfig randomRequestableGame = requestableGames[randomIndex];
        _ChallengeToRequest = randomRequestableGame.RequestBehaviorGameFlag;
        _ChallengeToRequestData = requestableGamesData[randomIndex];
        DAS.Info("GameToRequest " + _ChallengeToRequestData.ChallengeID, "");
      }
    }

    private void GetRequestableAndUnlockedGames(out List<RequestGameConfig> outRequestData, out List<ChallengeData> outChallengeData, out List<float> outRequestScores) {
      outRequestData = new List<RequestGameConfig>();
      outChallengeData = new List<ChallengeData>();
      outRequestScores = new List<float>();
      RequestGameListConfig requestGameConfig = RequestGameListConfig.Instance;
      ChallengeData data;
      int currentNumLightCubes = _RobotToTrack.LightCubes.Count;
      for (int i = 0; i < requestGameConfig.RequestList.Length; ++i) {
        data = ChallengeList.GetChallengeDataById(requestGameConfig.RequestList[i].ChallengeID);
        if (data != null
            && data.MinigameConfig.NumCubesRequired() <= currentNumLightCubes
            && UnlockablesManager.Instance.IsUnlocked(data.UnlockId.Value)) {
          bool isDailyGoal = DailyGoalManager.Instance.IsAnyGoalActiveForGame(requestGameConfig.RequestList[i].ChallengeID);
          float score = requestGameConfig.RequestList[i].GetCurrentScore(isDailyGoal, _IsDebugRequest);
          if (score > float.Epsilon) {
            outRequestData.Add(requestGameConfig.RequestList[i]);
            outChallengeData.Add(data);
            outRequestScores.Add(score);
          }
        }
      }
    }

    private void DebugSelectGameToRequest(string param) {
      _IsDebugRequest = true;
      EnableRequestGameBehaviorGroups();
      _IsDebugRequest = false;
    }
  }
}