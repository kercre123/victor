using UnityEngine;
using System.Collections;
using DataPersistence;

public class InventoryPane : MonoBehaviour {

  private static GameObject _sTrayInstance;

  [SerializeField]
  UnityEngine.UI.Button _QuickAddTreatButton;

  [SerializeField]
  UnityEngine.UI.Button _QuickRemoveTreatButton;

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

  [SerializeField]
  private UnityEngine.UI.Button _CreateInventoryTrayButton;

  [SerializeField]
  private UnityEngine.UI.Button _RemoveInventoryTrayButton;

  [SerializeField]
  private GameObject _InventoryTrayPrefab;

  private Cozmo.Inventory _PlayerInventory;

  private void Start() {
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

    _CreateInventoryTrayButton.onClick.AddListener(HandleCreateInventoryTrayButtonClicked);
    _RemoveInventoryTrayButton.onClick.AddListener(HandleRemoveInventoryTrayButtonClicked);
  }

  private void OnDestroy() {
    _QuickAddTreatButton.onClick.RemoveListener(HandleQuickAddTreatsClicked);
    _QuickRemoveTreatButton.onClick.RemoveListener(HandleQuickRemoveTreatsClicked);
    _AddItemButton.onClick.RemoveListener(HandleAddItemClicked);
    _RemoveItemButton.onClick.RemoveListener(HandleRemoveItemClicked);
    _SetItemAmountButton.onClick.RemoveListener(HandleSetItemAmountClicked);
    _CreateInventoryTrayButton.onClick.RemoveListener(HandleCreateInventoryTrayButtonClicked);
    _RemoveInventoryTrayButton.onClick.RemoveListener(HandleRemoveInventoryTrayButtonClicked);
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

  private void HandleCreateInventoryTrayButtonClicked() {
    if (_sTrayInstance == null) {
      _sTrayInstance = UIManager.CreateUIElement(_InventoryTrayPrefab, FindDebugCanvasParent(transform));
    }
  }

  private Transform FindDebugCanvasParent(Transform target) {
    if (target == null) {
      return null;
    }
    if (target.name == "DebugMenuCanvas") {
      return target;
    }
    return FindDebugCanvasParent(target.parent);
  }

  private void HandleRemoveInventoryTrayButtonClicked() {
    if (_sTrayInstance != null) {
      Destroy(_sTrayInstance);
    }
  }
}
