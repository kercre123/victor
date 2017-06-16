using UnityEngine;
using System.Collections;
using DataPersistence;

public class InventoryPane : MonoBehaviour {

  [SerializeField]
  UnityEngine.UI.Button _QuickAddTreatButton;

  [SerializeField]
  UnityEngine.UI.Button _QuickRemoveTreatButton;

  // TODO: Validate id
  [SerializeField]
  UnityEngine.UI.Dropdown _ItemIdDropdown;

  [SerializeField]
  UnityEngine.UI.InputField _NumToAddInputField;

  [SerializeField]
  UnityEngine.UI.Button _AddItemButton;

  [SerializeField]
  UnityEngine.UI.InputField _NumToRemoveInputField;

  [SerializeField]
  UnityEngine.UI.Button _RemoveItemButton;

  [SerializeField]
  UnityEngine.UI.InputField _NumToSetInputField;

  [SerializeField]
  UnityEngine.UI.Button _SetItemAmountButton;

  private Cozmo.Inventory _PlayerInventory;

  // TODO: Allow add / remove any item
  // TODO: Add shortcut buttons for common items (treats, experience)
  void Start() {
    _PlayerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;

    _QuickAddTreatButton.onClick.AddListener(HandleQuickAddTreatsClicked);
    _QuickRemoveTreatButton.onClick.AddListener(HandleQuickRemoveTreatsClicked);

    _ItemIdDropdown.ClearOptions();
    _ItemIdDropdown.AddOptions(Cozmo.ItemDataConfig.GetAllItemIds());

    string defaultValue = "10";
    _NumToAddInputField.contentType = UnityEngine.UI.InputField.ContentType.IntegerNumber;
    _NumToAddInputField.text = defaultValue;
    _AddItemButton.onClick.AddListener(HandleAddItemClicked);

    _NumToRemoveInputField.contentType = UnityEngine.UI.InputField.ContentType.IntegerNumber;
    _NumToRemoveInputField.text = defaultValue;
    _RemoveItemButton.onClick.AddListener(HandleRemoveItemClicked);

    _NumToSetInputField.contentType = UnityEngine.UI.InputField.ContentType.IntegerNumber;
    _NumToSetInputField.text = defaultValue;
    _SetItemAmountButton.onClick.AddListener(HandleSetItemAmountClicked);
  }

  private void HandleQuickAddTreatsClicked() {
    DebugAddItem(Cozmo.UI.GenericRewardsConfig.Instance.SparkID, 100);
  }

  private void HandleQuickRemoveTreatsClicked() {
    DebugRemoveItem(Cozmo.UI.GenericRewardsConfig.Instance.SparkID, 100);
  }

  private void HandleAddItemClicked() {
    DebugAddItem(_ItemIdDropdown.captionText.text, GetValidInt(_NumToAddInputField));
  }

  private void HandleRemoveItemClicked() {
    DebugRemoveItem(_ItemIdDropdown.captionText.text, GetValidInt(_NumToRemoveInputField));
  }

  private void HandleSetItemAmountClicked() {
    DebugSetItem(_ItemIdDropdown.captionText.text, GetValidInt(_NumToSetInputField));
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

  private void DebugSetItem(string itemId, int delta) {
    _PlayerInventory.SetItemAmount(itemId, delta);
    DataPersistenceManager.Instance.Save();
  }

  private int GetValidInt(UnityEngine.UI.InputField inputField) {
    int validInt = int.Parse(inputField.text);
    if (validInt < 0) {
      validInt = 0;
      inputField.text = "0";
    }
    return validInt;
  }
}
