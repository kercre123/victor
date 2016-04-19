using UnityEngine;
using System.Collections;
using Cozmo.UI;
using DataPersistence;
using UnityEngine.Serialization;

public class RequestTricksView : BaseView {

  [SerializeField]
  private CozmoButton _TrickRequestButton;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _UnlockableDescription;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _UnlockableTitle;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _UnlockableState;

  private UnlockableInfo _UnlockInfo;
  private CozmoUnlocksPanel _CozmoUnlocksPanel;

  public void Initialize(UnlockableInfo unlockInfo, CozmoUnlocksPanel cozmoUnlocksPanel) {
    _UnlockInfo = unlockInfo;
    _CozmoUnlocksPanel = cozmoUnlocksPanel;
    _TrickRequestButton.onClick.AddListener(OnSparkClicked);

    Cozmo.ItemData itemData = Cozmo.ItemDataConfig.GetData(_UnlockInfo.CostItemId);
    string itemNamePlural = (itemData != null) ? itemData.GetPluralName() : "(null)";
    _TrickRequestButton.FormattingArgs = new object[] { _UnlockInfo.CostAmountNeeded, itemNamePlural };

    _UnlockableDescription.text = Localization.Get(unlockInfo.DescriptionKey);
    _UnlockableTitle.text = Localization.Get(unlockInfo.TitleKey);
    _TrickRequestButton.DASEventButtonName = "trick_request_button";
    _TrickRequestButton.DASEventViewController = "request_tricks_view";
    UpdateState();
    RobotEngineManager.Instance.OnSparkUnlockEnded += HandleSparkUnlockEnded;
  }

  private void UpdateState() {
    IRobot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot.IsSparked && robot.SparkUnlockId == _UnlockInfo.Id.Value) {
      _TrickRequestButton.Interactable = false;
    }
    else {
      _TrickRequestButton.Interactable = true;
    }
  }

  private void OnSparkClicked() {
    Cozmo.Inventory playerInventory = DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
    if (playerInventory.CanRemoveItemAmount(_UnlockInfo.CostItemId, _UnlockInfo.CostAmountNeeded)) {
      playerInventory.RemoveItemAmount(_UnlockInfo.CostItemId, _UnlockInfo.CostAmountNeeded);
      RobotEngineManager.Instance.CurrentRobot.EnableSparkUnlock(_UnlockInfo.Id.Value);
      // update UI for the cozmo unlocks panel for treat count.
      _CozmoUnlocksPanel.UpdateInventoryCounts();
      UpdateState();
    }
  }

  private void HandleSparkUnlockEnded() {
    UpdateState();
  }

  protected override void CleanUp() {
    RobotEngineManager.Instance.CurrentRobot.StopSparkUnlock();
  }

}
