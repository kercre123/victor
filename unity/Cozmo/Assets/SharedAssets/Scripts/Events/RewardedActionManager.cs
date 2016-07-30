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
  public static RewardedActionManager Instance { get; private set; }

  public static string sRewardedActionsDirectory { get { return PlatformUtil.GetResourcesFolder("RewardedActions"); } }

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

  [SerializeField]
  private GenericRewardsConfig _RewardConfig;

  private RewardedActionListData _RewardData;

  // Map Animation Group Names to Event Enums using the tool
  public Dictionary<GameEvent, List<RewardedActionData>> RewardEventMap = new Dictionary<GameEvent, List<RewardedActionData>>();

  // Rewards that have been earned but haven't been shown to the player.
  public Dictionary<RewardedActionData, int> PendingActionRewards = new Dictionary<RewardedActionData, int>();

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
      foreach (RewardedActionData reward in RewardedActionManager.Instance.PendingActionRewards.Keys) {
        int count = 0;
        if (RewardedActionManager.Instance.PendingActionRewards.TryGetValue(reward, out count)) {
          if (reward.Reward.ItemID == _RewardConfig.EnergyID) {
            total += count;
          }
        }
      }
      return total;
    }
  }


  void Start() {
    RegisterEvents();
    LoadData();
  }

  private void LoadData() {
    // Load all Event Map Configs (Can have multiple, so you can create different configs, game only uses one.)
    if (Directory.Exists(sRewardedActionsDirectory)) {
      string[] _RewardedActionFiles = Directory.GetFiles(sRewardedActionsDirectory);
      if (_RewardedActionFiles.Length > 0) {
        bool didMatch = false;
        for (int i = 0; i < _RewardedActionFiles.Length; i++) {
          if (_RewardedActionFiles[i] == Path.Combine(sRewardedActionsDirectory, _RewardConfig.RewardConfigFilename)) {
            LoadRewardedActions(_RewardedActionFiles[i]);
            didMatch = true;
            break;
          }
        }
        if (!didMatch) {
          DAS.Error(this, string.Format("No Reward Data to load in {0}", sRewardedActionsDirectory));
        }
      }
      else {
        DAS.Error(this, string.Format("No Reward Data to load in {0}", sRewardedActionsDirectory));
      }
    }
    else {
      DAS.Error(this, string.Format("No Reward Data to load in {0}", sRewardedActionsDirectory));
    }

  }

  void OnDestroy() {
    DeregisterEvents();
  }

  public void GameEventReceived(GameEventWrapper cozEvent) {
    List<RewardedActionData> rewardList = new List<RewardedActionData>();
    if (RewardEventMap.TryGetValue(cozEvent.GameEventEnum, out rewardList)) {
      for (int i = 0; i < rewardList.Count; i++) {
        RewardedActionData reward = rewardList[i];
        if (RewardConditionsMet(reward, cozEvent)) {
          DAS.Info(this, string.Format("{0} rewarded {1} {2}", cozEvent.GameEventEnum, reward.Reward.Amount, reward.Reward.ItemID));
          DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.AddItemAmount(reward.Reward.ItemID, reward.Reward.Amount);
          if (!PendingActionRewards.ContainsKey(reward)) {
            PendingActionRewards.Add(reward, reward.Reward.Amount);
          }
          else {
            PendingActionRewards[reward] += reward.Reward.Amount;
          }
        }
      }
    }
  }

  public bool RewardConditionsMet(RewardedActionData reward, GameEventWrapper gEvent) {
    if (reward.EventConditions == null) {
      return true;
    }
    for (int i = 0; i < reward.EventConditions.Count; i++) {
      if (reward.EventConditions[i].ConditionMet(gEvent) == false) {
        return false;
      }
    }
    return true;
  }

  private void LoadRewardedActions(string path) {
    DAS.Event(this, string.Format("LoadRewardedActionData from {0}", Path.GetFileName(path)));
    string json = File.ReadAllText(path);

    GameEvent gEvent;
    RewardedActionData reward;
    List<RewardedActionData> rList;
    _RewardData = JsonConvert.DeserializeObject<RewardedActionListData>(json, GlobalSerializerSettings.JsonSettings);
    for (int i = 0; i < _RewardData.RewardedActions.Count; i++) {
      gEvent = _RewardData.RewardedActions[i].RewardEvent.Value;
      reward = _RewardData.RewardedActions[i];
      // If there isn't a reward for this game event, create a new list
      if (!RewardEventMap.ContainsKey(gEvent)) {
        rList = new List<RewardedActionData>();
        rList.Add(reward);
        RewardEventMap.Add(gEvent, rList);
      }
      else {
        // Otherwise add this RewardedAction to the end of the list
        RewardEventMap[gEvent].Add(reward);
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
