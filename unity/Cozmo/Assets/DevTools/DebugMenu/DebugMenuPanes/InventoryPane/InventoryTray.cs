using DataPersistence;
using UnityEngine;

public class InventoryTray : MonoBehaviour {

  [SerializeField]
  UnityEngine.UI.Button _QuickAddTreatButton;

  [SerializeField]
  UnityEngine.UI.Button _QuickRemoveTreatButton;

  private Cozmo.Inventory _PlayerInventory;

  private void Start() {
    _PlayerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;

    _QuickAddTreatButton.onClick.AddListener(HandleQuickAddTreatsClicked);
    _QuickRemoveTreatButton.onClick.AddListener(HandleQuickRemoveTreatsClicked);
  }

  private void OnDestroy() {
    _QuickAddTreatButton.onClick.RemoveListener(HandleQuickAddTreatsClicked);
    _QuickRemoveTreatButton.onClick.RemoveListener(HandleQuickRemoveTreatsClicked);
  }

  private void HandleQuickAddTreatsClicked() {
    DebugAddItem(Cozmo.UI.GenericRewardsConfig.Instance.SparkID, 20);
  }

  private void HandleQuickRemoveTreatsClicked() {
    DebugRemoveItem(Cozmo.UI.GenericRewardsConfig.Instance.SparkID, 20);
  }

  private void DebugAddItem(string itemId, int delta) {
    // Set to Fake Reward Pending if energy so it will behave like normal energy earned
    // for loot view testing and whatnot
    // If not an energy item then instantly add to inventory
    if (delta > 0 && itemId == RewardedActionManager.Instance.EnergyID) {
      RewardedActionManager.Instance.FakeRewardPending(delta);
    }
    else {
      _PlayerInventory.AddItemAmount(itemId, delta);
    }
    DataPersistenceManager.Instance.Save();
  }

  private void DebugRemoveItem(string itemId, int delta) {
    int currentAmount = _PlayerInventory.GetItemAmount(itemId);
    delta = Mathf.Min(currentAmount, delta);
    _PlayerInventory.RemoveItemAmount(itemId, delta);
    DataPersistenceManager.Instance.Save();
  }
}
