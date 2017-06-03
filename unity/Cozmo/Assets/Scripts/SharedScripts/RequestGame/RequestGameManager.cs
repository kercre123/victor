using Anki.Cozmo;
using Anki.Debug;
using Cozmo.Challenge;
using System.Collections.Generic;
using UnityEngine;
using Anki.Cozmo.ExternalInterface;

namespace Cozmo.RequestGame {
  public class RequestGameManager {

    private ChallengeDataList ChallengeList {
      get { return ChallengeDataList.Instance; }
    }

    public void EnableRequestGameBehaviorGroups() {
      RobotEngineManager.Instance.Message.CanCozmoRequestGame = Singleton<CanCozmoRequestGame>.Instance.Initialize(true);
      RobotEngineManager.Instance.SendMessage();
    }

    public void DisableRequestGameBehaviorGroups() {
      RobotEngineManager.Instance.Message.CanCozmoRequestGame = Singleton<CanCozmoRequestGame>.Instance.Initialize(false);
      RobotEngineManager.Instance.SendMessage();
    }

    public ChallengeData GetDataForGameID(UnlockId id) {
      // Mark always once the popup shows up, after that should show the others
      if (RequestGameListConfig.Instance != null) {
        RequestGameListConfig requestGameConfig = RequestGameListConfig.Instance;
        for (int i = 0; i < requestGameConfig.RequestList.Length; ++i) {
          if (requestGameConfig.RequestList[i].RequestGameID.Value.Equals(id)) {
            return ChallengeList.GetChallengeDataById(requestGameConfig.RequestList [i].ChallengeID);
          }
        }
      }
      return null;
    }
  }
}