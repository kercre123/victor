using System;
using UnityEngine;
using Anki.Cozmo;
using Cozmo.UI;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json;
using System.IO;
using DataPersistence;

/// <summary>
/// Manager for granting rewards that are mapped to game events
/// </summary>
public class RewardedActionManager : MonoBehaviour {

  // Disable if we don't want to reward points, for instance, if we want minigames to ignore these
  // Just make sure you reenable before resolving anything that would trigger end of game events
  // (for the sake of win speed tap esque goals.
  public bool RewardsEnabled = true;

  private static RewardedActionManager _Instance = null;

  public static RewardedActionManager Instance { 
    get { 
      if (_Instance == null) { 
        _Instance = new RewardedActionManager();
      }
      return _Instance; 
    }
    set {
      if (_Instance != null) {
        _Instance = value;
      }
    }
  }

  private IRobot _CurrRobot = null;

  public IRobot CurrentRobot {
    get {
      if (_CurrRobot == null) {
        _CurrRobot = RobotEngineManager.Instance.CurrentRobot;
      }
      return _CurrRobot;
    }
  }

  // TODO: Replace config with json
  [SerializeField]
  private GenericRewardsConfig _RewardConfig;

  // Map Animation Group Names to Event Enums using the tool
  public Dictionary<GameEvent, int> RewardEventMap = new Dictionary<GameEvent, int>();


  void Start() {
    GameEventManager.Instance.OnGameEvent += GameEventReceived;
    // TODO: Load in config file used for handling this
    GameEvent gEvent;
    int amount;
    for (int i = 0; i < _RewardConfig.RewardedActions.Count; i++) {
      gEvent = _RewardConfig.RewardedActions[i].GameEvent;
      amount = _RewardConfig.RewardedActions[i].Amount;
      if (!RewardEventMap.ContainsKey(gEvent)) {
        RewardEventMap.Add(gEvent, amount);
      }
      else {
        DAS.Error(this, string.Format("{0} is a redundant event. Already have a reward for this.", gEvent));
      }
    }
  }

  void OnDestroy() {
    GameEventManager.Instance.OnGameEvent -= GameEventReceived;
  }

  public void GameEventReceived(GameEventWrapper cozEvent) {
    int reward = 0;
    if (enabled) {
      if (RewardEventMap.TryGetValue(cozEvent.GameEventEnum, out reward)) {
        DAS.Info(this, string.Format("{0} rewarded {1} points", cozEvent.GameEventEnum, reward));
        DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.AddItemAmount("experience", reward);
      }
    }
  }


}
