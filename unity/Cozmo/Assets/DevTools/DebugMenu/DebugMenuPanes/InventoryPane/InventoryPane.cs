using UnityEngine;
using System.Collections;
using DataPersistence;

public class InventoryPane : MonoBehaviour {

  [SerializeField]
  UnityEngine.UI.Button _QuickAddTreatButton;

  [SerializeField]
  UnityEngine.UI.Button _QuickRemoveTreatButton;

  [SerializeField]
  UnityEngine.UI.Button _QuickAddExperienceButton;

  [SerializeField]
  UnityEngine.UI.Button _QuickRemoveExperienceButton;

  [SerializeField]
  UnityEngine.UI.InputField _ChestLevelInputField;

  [SerializeField]
  UnityEngine.UI.Button _SetChestLevelButton;

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

  private const string _kTreatItemId = "treat";
  private const string _kExperienceItemId = "experience";

  private Cozmo.Inventory _PlayerInventory;

  // TODO: Allow add / remove any item
  // TODO: Add shortcut buttons for common items (treats, experience)
  void Start() {
    _PlayerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;

    _QuickAddTreatButton.onClick.AddListener(HandleQuickAddTreatsClicked);
    _QuickRemoveTreatButton.onClick.AddListener(HandleQuickRemoveTreatsClicked);
    _QuickAddExperienceButton.onClick.AddListener(HandleQuickAddExperienceClicked);
    _QuickRemoveExperienceButton.onClick.AddListener(HandleQuickRemoveExperienceClicked);
    if (DataPersistenceManager.Instance.CurrentSession != null) {
      _ChestLevelInputField.text = DataPersistenceManager.Instance.CurrentSession.ChestsGained.ToString();
    }
    _ChestLevelInputField.contentType = UnityEngine.UI.InputField.ContentType.IntegerNumber;
    _SetChestLevelButton.onClick.AddListener(HandleSetChestLevelButtonClicked);

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
    DebugAddItem(_kTreatItemId, 100);
  }

  private void HandleQuickRemoveTreatsClicked() {
    DebugRemoveItem(_kTreatItemId, 100);
  }

  private void HandleQuickAddExperienceClicked() {
    DebugAddItem(_kExperienceItemId, 100);
  }

  private void HandleQuickRemoveExperienceClicked() {
    DebugRemoveItem(_kExperienceItemId, 100);
  }

  private void HandleSetChestLevelButtonClicked() {
    if (DataPersistenceManager.Instance.CurrentSession != null) {
      DataPersistenceManager.Instance.CurrentSession.ChestsGained = GetValidInt(_ChestLevelInputField);

      // Trigger UI to update
      DebugAddItem(_kExperienceItemId, 0);

      DataPersistenceManager.Instance.Save();
    }
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
    _PlayerInventory.AddItemAmount(itemId, delta);
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
