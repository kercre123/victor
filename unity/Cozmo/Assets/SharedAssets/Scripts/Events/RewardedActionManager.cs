using System;
using UnityEngine;
using Anki.Cozmo;
using System.Collections;
using System.Collections.Generic;
using Newtonsoft.Json;
using System.IO;
using DataPersistence;

/// <summary>
/// Manager for granting rewards that are mapped to game events
/// </summary>
public class RewardManager {

  // Disable if we don't want to reward points, for instance, if we want minigames to ignore these
  // Just make sure you reenable before resolving anything that would trigger end of game events
  // (for the sake of win speed tap esque goals.
  public bool enabled = true;

  private static RewardManager _Instance = null;

  public static RewardManager Instance { 
    get { 
      if (_Instance == null) { 
        _Instance = new RewardManager();
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

  // Map Animation Group Names to Event Enums using the tool
  public Dictionary<GameEvent, int> RewardEventMap = new Dictionary<GameEvent, int>();


  public RewardManager() {
    GameEventManager.Instance.OnGameEvent += GameEventReceived;
    // TODO: Load in config file used for handling this
  }

  public void GameEventReceived(GameEvent cozEvent) {
    int reward = 0;
    if (enabled) {
      if (RewardEventMap.TryGetValue(cozEvent, out reward)) {
        DAS.Info(this, string.Format("{0} rewarded {1} points", cozEvent, reward));
        DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.AddItemAmount("experience", reward);
      }
    }
  }


}
