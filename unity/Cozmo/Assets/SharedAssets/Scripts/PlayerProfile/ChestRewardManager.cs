using UnityEngine;
using System.Collections;
using System.Linq;
using DataPersistence;

public class ChestRewardManager {

  private static ChestRewardManager _sInstance;

  public static ChestRewardManager Instance { get { return _sInstance; } }

  public static void CreateInstance() {
    _sInstance = new ChestRewardManager();
  }

  public delegate void ChestRequirementsUpdateHandler(int points,int maxPoints);

  public delegate void ChestGainedHandler();

  public ChestRequirementsUpdateHandler ChestRequirementsGained;

  public ChestGainedHandler ChestGained;

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

  private ChestData GetChestData() {
    return ChestData.Instance;
  }

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
    return value;
  }

  private void HandleItemValueChanged(string itemId, int delta, int newCount) {
    if (itemId != GetChestData().RequirementLadder.ItemId) {
      return;
    }

    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    DeregisterEvents(playerInventory);

    LadderLevel[] requirementLadderLevels = GetChestData().RequirementLadder.LadderLevels;
    int currentLadderMax = GetCurrentLadderValue(requirementLadderLevels);
    while (newCount >= currentLadderMax) {
      int rewardAmount = 0;
      foreach (Ladder ladder in GetChestData().RewardLadders) {
        rewardAmount = GetCurrentLadderValue(ladder.LadderLevels);
        playerInventory.AddItemAmount(ladder.ItemId, rewardAmount);
      }

      DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault().ChestsGained++;

      if (ChestGained != null) {
        ChestGained();
      }

      newCount -= currentLadderMax;
      currentLadderMax = GetCurrentLadderValue(requirementLadderLevels);
    }

    playerInventory.SetItemAmount(itemId, newCount);

    if (ChestRequirementsGained != null) {
      ChestRequirementsGained(newCount, currentLadderMax);
    }

    RegisterEvents(playerInventory);
  }

  private void RegisterEvents(Cozmo.Inventory inventory) {
    inventory.ItemAdded += HandleItemValueChanged;
    inventory.ItemRemoved += HandleItemValueChanged;
    inventory.ItemCountSet += HandleItemValueChanged;
  }

  private void DeregisterEvents(Cozmo.Inventory inventory) {
    inventory.ItemAdded -= HandleItemValueChanged;
    inventory.ItemRemoved -= HandleItemValueChanged;
    inventory.ItemCountSet -= HandleItemValueChanged;
  }
}
