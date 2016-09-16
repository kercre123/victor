using UnityEngine;
using System.Collections;
using DataPersistence;

public class FaceEnrollmentUnlockView : Cozmo.UI.BaseView {

  [SerializeField]
  private Color _AvailableColor;
  [SerializeField]
  private Color _UnavailableColor;

  [SerializeField]
  private Cozmo.UI.CozmoButton _UnlockButton;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _AvailabilityText;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _UnlockCostText;

  private string _ItemId;
  private int _UnlockCost;

  private void Awake() {
    _UnlockButton.Initialize(HandleOnUnlockButton, "unlock_button", this.DASEventViewName);
  }

  public void Initialize(string itemId, int unlockCost) {
    _ItemId = itemId;
    _UnlockCost = unlockCost;
    _UnlockButton.Interactable = IsAffordable();
    if (IsAffordable()) {
      _AvailabilityText.text = Localization.Get(LocalizationKeys.kLabelAvailable);
      _AvailabilityText.color = _AvailableColor;
      _UnlockCostText.color = _UnlockButton.TextEnabledColor;
    }
    else {
      _AvailabilityText.text = Localization.Get(LocalizationKeys.kUnlockableBitsRequiredTitle);
      _AvailabilityText.color = _UnavailableColor;
      _UnlockCostText.color = _UnlockButton.TextDisabledColor;
    }

    _UnlockCostText.text = Localization.GetWithArgs(LocalizationKeys.kLabelEmptyWithArg, unlockCost);
  }

  private void HandleOnUnlockButton() {
    UnlockablesManager.Instance.UnlockNextAvailableFaceSlot();
    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    playerInventory.RemoveItemAmount(_ItemId, _UnlockCost);
    CloseView();
  }

  private bool IsAffordable() {
    Cozmo.Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    bool affordable = playerInventory.CanRemoveItemAmount(_ItemId, _UnlockCost);
    return affordable;
  }

  protected override void CleanUp() {
    base.CleanUp();
  }
}
