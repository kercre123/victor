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
    return DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.GetItemAmount(GetChestData().Requirement.ItemId);
  }

  public int GetNextRequirementPoints() {
    return GetChestData().Requirement.TargetPoints;
  }

  public string ChestRequirementItemID {
    get {
      return GetChestData().Requirement.ItemId;
    }
  }

  private Dictionary<string, int> _PendingDeductions = new Dictionary<string, int>();

  private ChestData GetChestData() {
    return ChestData.Instance;
  }

  // checks if we need to populate the chest with rewards, if we have more of the requirement item id in inventory
  // than the target points, then treat that as a chest earned. If we are earning multiple chests at once, collapse
  // all their rewards into one uberchest to avoid cludgy user feedback
  public void TryPopulateChestRewards() {
    string itemId = GetChestData().Requirement.ItemId;
    int pendingDeductions = 0;
    _PendingDeductions.TryGetValue(itemId, out pendingDeductions);
    int itemCount = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory.GetItemAmount(itemId) - pendingDeductions;

    // If we've earned multiple chests at once through getting an absurd number of points
    // collapse all rewarded chests and adjust points accordingly.
    while (itemCount >= GetChestData().Requirement.TargetPoints) {
      int rewardAmount = 0;
      foreach (ChestRewardData chest in GetChestData().RewardList) {
        rewardAmount = UnityEngine.Random.Range(chest.MinAmount, chest.MaxAmount);
        if (PendingChestRewards.ContainsKey(chest.ItemId)) {
          PendingChestRewards[chest.ItemId] += rewardAmount;
        }
        else {
          PendingChestRewards.Add(chest.ItemId, rewardAmount);
        }
      }

      DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault().ChestsGained++;
      if (ChestGained != null) {
        ChestGained();
      }

      if (_PendingDeductions.ContainsKey(itemId)) {
        _PendingDeductions[itemId] += GetChestData().Requirement.TargetPoints;
      }
      else {
        _PendingDeductions.Add(itemId, GetChestData().Requirement.TargetPoints);
      }

      itemCount -= GetChestData().Requirement.TargetPoints;
    }

    if (ChestRequirementsGained != null) {
      ChestRequirementsGained(itemCount, GetChestData().Requirement.TargetPoints);
    }

  }

  private void HandleItemValueChanged(string itemId, int delta, int newCount) {
    if (itemId == GetChestData().Requirement.ItemId) {
      DAS.Event("meta.emotion_chip.state", delta.ToString(), DASUtil.FormatExtraData(newCount.ToString()));
      // see if we need to populate the chest every time we get value updates from the
      // requirement ladder item type.
      TryPopulateChestRewards();
    }
  }

  public void ApplyChestRewards() {
    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    foreach (KeyValuePair<string, int> kvp in PendingChestRewards) {
      DAS.Event("meta.emotion_chip.open", kvp.Key, DASUtil.FormatExtraData(kvp.Value.ToString()));
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
