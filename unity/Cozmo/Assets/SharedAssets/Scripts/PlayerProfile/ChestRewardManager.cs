using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using DataPersistence;

public class ChestRewardManager {

  private static ChestRewardManager _sInstance;

  public static ChestRewardManager Instance { get { return _sInstance; } }

  public static void CreateInstance() {
    _sInstance = new ChestRewardManager();
  }

  public delegate void ChestRequirementsUpdateHandler(int points, int maxPoints);

  public delegate void ChestGainedHandler();

  public ChestRequirementsUpdateHandler ChestRequirementsGained;

  public ChestGainedHandler ChestGained;

  // Rewards that have been earned but haven't been shown to the player.
  public Dictionary<string, int> PendingChestRewards = new Dictionary<string, int>();

  public bool ChestPending {
    get {
      return PendingChestRewards.Count > 0;
    }
  }

  public ChestRewardManager() {
    // Listen for inventory values changing
    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    RegisterEvents(playerInventory);
  }

  public int GetCurrentRequirementPoints() {
    return DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.GetItemAmount(GetChestData().RequirementLadder.ItemId);
  }

  public int GetNextRequirementPoints() {
    return GetCurrentLadderValue(GetChestData().RequirementLadder.LadderLevels);
  }

  public int GetPreviousRequirementPoints() {
    return GetPreviousLadderValue(GetChestData().RequirementLadder.LadderLevels);
  }

  public string ChestRequirementItemID {
    get {
      return GetChestData().RequirementLadder.ItemId;
    }
  }

  private Dictionary<string, int> _PendingDeductions = new Dictionary<string, int>();

  private ChestData GetChestData() {
    return ChestData.Instance;
  }

  /// <summary>
  /// Gets the points needed to earn the current chest.
  /// </summary>
  /// <returns>The current ladder value.</returns>
  /// <param name="ladderLevels">Ladder levels.</param>
  private int GetCurrentLadderValue(LadderLevel[] ladderLevels) {
    int ladderLevel = 0;
    if (DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault() != null) {
      ladderLevel = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault().ChestsGained;
    }
    int value = ladderLevels.Last().Value; // default to max value
    for (int i = 0; i < ladderLevels.Length - 1; ++i) {
      if (ladderLevel >= ladderLevels[i].Level && ladderLevel < ladderLevels[i + 1].Level) {
        value = ladderLevels[i].Value;
        break;
      }
    }
    if (value == 0) {
      DAS.Error("ChestRewardManager.GetCurrentLadderValue", "LadderValue should never be 0");
      value = 1;
    }
    return value;
  }

  /// <summary>
  /// Gets the points needed to earn the previous chest.
  /// </summary>
  /// <returns>The current ladder value.</returns>
  /// <param name="ladderLevels">Ladder levels.</param>
  private int GetPreviousLadderValue(LadderLevel[] ladderLevels) {
    int ladderLevel = 0;
    if (DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault() != null) {
      ladderLevel = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault().ChestsGained;
    }
    int value = ladderLevels.First().Value; // default to first value
    for (int i = 0; i < ladderLevels.Length; ++i) {
      if ((ladderLevel - 1) == ladderLevels[i].Level) {
        value = ladderLevels[i].Value;
        break;
      }
    }
    if (value == 0) {
      DAS.Error("ChestRewardManager.GetPreviousLadderValue", "LadderValue should never be 0");
      value = 1;
    }
    return value;
  }

  // checks if we need to populate the chest with rewards
  public void TryPopulateChestRewards() {
    string itemId = GetChestData().RequirementLadder.ItemId;
    int itemCount = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.GetItemAmount(itemId);

    LadderLevel[] requirementLadderLevels = GetChestData().RequirementLadder.LadderLevels;
    int currentLadderMax = GetCurrentLadderValue(requirementLadderLevels);

    while (itemCount >= currentLadderMax) {
      int rewardAmount = 0;
      foreach (Ladder ladder in GetChestData().RewardLadders) {
        rewardAmount = GetCurrentLadderValue(ladder.LadderLevels);
        if (PendingChestRewards.ContainsKey(ladder.ItemId)) {
          PendingChestRewards[ladder.ItemId] += rewardAmount;
        }
        else {
          PendingChestRewards.Add(ladder.ItemId, rewardAmount);
        }
      }

      DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault().ChestsGained++;
      if (ChestGained != null) {
        ChestGained();
      }

      if (_PendingDeductions.ContainsKey(itemId)) {
        _PendingDeductions[itemId] += currentLadderMax;
      }
      else {
        _PendingDeductions.Add(itemId, currentLadderMax);
      }

      itemCount -= currentLadderMax;

      currentLadderMax = GetCurrentLadderValue(requirementLadderLevels);
    }

    if (ChestRequirementsGained != null) {
      ChestRequirementsGained(itemCount, currentLadderMax);
    }

  }

  private void HandleItemValueChanged(string itemId, int delta, int newCount) {
    if (itemId == GetChestData().RequirementLadder.ItemId) {
      // see if we need to populate the chest every time we get value updates from the
      // requirement ladder item type.
      TryPopulateChestRewards();
    }
  }

  public void ApplyChestRewards() {
    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    foreach (KeyValuePair<string, int> kvp in PendingChestRewards) {
      playerInventory.AddItemAmount(kvp.Key, kvp.Value);
    }
    PendingChestRewards.Clear();

    foreach (KeyValuePair<string, int> kvp in _PendingDeductions) {
      playerInventory.RemoveItemAmount(kvp.Key, kvp.Value);
    }
    _PendingDeductions.Clear();
    DataPersistence.DataPersistenceManager.Instance.Save();
  }

  private void RegisterEvents(Cozmo.Inventory inventory) {
    inventory.ItemAdded += HandleItemValueChanged;
    inventory.ItemRemoved += HandleItemValueChanged;
    inventory.ItemCountSet += HandleItemValueChanged;
  }

}
