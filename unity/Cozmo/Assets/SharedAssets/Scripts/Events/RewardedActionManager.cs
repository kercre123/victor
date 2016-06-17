using System;
using UnityEngine;
using Anki.Cozmo;
using Cozmo;
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

  [ItemId]
  public string ActionRewardID;

  public static RewardedActionManager Instance { get; private set; }

  private IRobot _CurrRobot = null;

  public IRobot CurrentRobot {
    get {
      if (_CurrRobot == null) {
        _CurrRobot = RobotEngineManager.Instance.CurrentRobot;
      }
      return _CurrRobot;
    }
  }

  private void OnEnable() {
    if (Instance != null && Instance != this) {
      Destroy(gameObject);
      return;
    }
    else {
      Instance = this;
    }
  }

  // TODO: Replace config with json
  [SerializeField]
  private GenericRewardsConfig _RewardConfig;

  // Map Animation Group Names to Event Enums using the tool
  public Dictionary<GameEvent, RewardData> RewardEventMap = new Dictionary<GameEvent, RewardData>();

  // Rewards that have been earned but haven't been shown to the player.
  public Dictionary<GameEvent, int> PendingActionRewards = new Dictionary<GameEvent, int>();

  public bool RewardPending {
    get {
      return PendingActionRewards.Count > 0;
    }
  }
  // Set to -1 if no new difficulty was unlocked, otherwise is set to the new difficulty and included along
  // with Pending Unlockables as the next difficulty unlocked.
  public int NewDifficultyUnlock = -1;

  public bool NewDifficultyPending {
    get {
      return NewDifficultyUnlock != -1;
    }
  }

  public int TotalPendingEnergy {
    get {
      int total = 0;
      foreach (GameEvent eventID in RewardedActionManager.Instance.PendingActionRewards.Keys) {
        int count = 0;
        if (RewardedActionManager.Instance.PendingActionRewards.TryGetValue(eventID, out count)) {
          total += count;
        }
      }
      return total;
    }
  }


  void Start() {
    RegisterEvents();
    GameEvent gEvent;
    RewardData reward;
    for (int i = 0; i < _RewardConfig.RewardedActions.Count; i++) {
      gEvent = _RewardConfig.RewardedActions[i].GameEvent.Value;
      reward = _RewardConfig.RewardedActions[i].Reward;
      if (!RewardEventMap.ContainsKey(gEvent)) {
        RewardEventMap.Add(gEvent, reward);
      }
      else {
        DAS.Error(this, string.Format("{0} is a redundant event. Already have a reward for this.", gEvent));
      }
    }
  }

  void OnDestroy() {
    DeregisterEvents();
  }

  public void GameEventReceived(GameEventWrapper cozEvent) {
    RewardData reward = null;
    if (RewardEventMap.TryGetValue(cozEvent.GameEventEnum, out reward)) {
      DAS.Info(this, string.Format("{0} rewarded {1} {2}", cozEvent.GameEventEnum, reward.Amount, reward.ItemID));
      DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.AddItemAmount(reward.ItemID, reward.Amount);
      if (reward.ItemID == ActionRewardID) {
        if (!PendingActionRewards.ContainsKey(cozEvent.GameEventEnum)) {
          PendingActionRewards.Add(cozEvent.GameEventEnum, reward.Amount);
        }
        else {
          PendingActionRewards[cozEvent.GameEventEnum] += reward.Amount;
        }
      }
    }
  }


  private void RegisterEvents() {
    GameEventManager.Instance.OnGameEvent += GameEventReceived;
  }

  private void DeregisterEvents() {
    GameEventManager.Instance.OnGameEvent -= GameEventReceived;
  }


}
