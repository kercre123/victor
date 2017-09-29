using UnityEngine;
using Cozmo.UI;

namespace Cozmo.Needs.Sparks.UI {
  public class SparkCell : MonoBehaviour {

    private const float kComingSoonAlpha = 1f;

    [SerializeField]
    protected CozmoImage _TrickIcon;

    [SerializeField]
    protected CozmoText _TrickTitleText;

    [SerializeField]
    protected CozmoButton _SparksButton;

    [SerializeField]
    protected CozmoText _SparkCountText;

    [SerializeField]
    private GameObject _SparkCostContainer;

    [SerializeField]
    protected SparksDetailModal _SparksDetailModalPrefab;

    [SerializeField]
    private Sprite _UnlockableAlertIcon;

    protected UnlockableInfo _UnlockInfo;
    protected CostLabel _CostLabelHelper;

    public virtual void Initialize(UnlockableInfo unlockInfo, string dasEventDialogName) {
      _UnlockInfo = unlockInfo;

      if (UnlockablesManager.Instance.IsUnlocked(unlockInfo.Id.Value)) {
        _SparksButton.Initialize(HandleTappedUnlocked, "spark_cell_" + unlockInfo.DASName, dasEventDialogName);
        _TrickIcon.sprite = unlockInfo.CoreUpgradeIcon;
        _TrickTitleText.text = Localization.Get(unlockInfo.TitleKey);

        _CostLabelHelper = new CostLabel(unlockInfo.RequestTrickCostItemId,
                       unlockInfo.RequestTrickCostAmount,
                                         _SparkCountText,
                                         UIColorPalette.GeneralSparkTintColor);
      }
      else {
        _SparksButton.Initialize(HandleTappedComingSoon, "spark_cell_coming_soon_" + unlockInfo.DASName, dasEventDialogName);
        _TrickIcon.color = new Color(_TrickIcon.color.r, _TrickIcon.color.g, _TrickIcon.color.b, kComingSoonAlpha);
        _SparkCostContainer.gameObject.SetActive(false);
        _TrickIcon.sprite = _UnlockableAlertIcon;
        _TrickTitleText.text = Localization.Get(LocalizationKeys.kUnlockableComingSoonTitle);
      }
    }

    private void OnDestroy() {
      if (_CostLabelHelper != null) {
        _CostLabelHelper.DeregisterEvents();
      }
    }

    private void HandleTappedComingSoon() {
      var cozmoNotReadyData = new AlertModalData("coming_soon_sparks_cell",
                                                 LocalizationKeys.kUnlockableComingSoonTitle,
                                                 LocalizationKeys.kUnlockableComingSoonDescription,
                                           new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose),
                                                icon: _UnlockableAlertIcon);

      ModalPriorityData comingSoonPriority = new ModalPriorityData(ModalPriorityLayer.VeryLow,
                                         0,
                                         LowPriorityModalAction.CancelSelf,
                                         HighPriorityModalAction.Stack);

      UIManager.OpenAlert(cozmoNotReadyData, comingSoonPriority);
    }

    protected virtual void HandleTappedUnlocked() {
      // pop up sparks modal
      UIManager.OpenModal(_SparksDetailModalPrefab, new ModalPriorityData(), (obj) => {
        SparksDetailModal sparksDetailModal = (SparksDetailModal)obj;
        sparksDetailModal.InitializeSparksDetailModal(_UnlockInfo, isEngineDriven: false);
      });
    }
  }

  public class CostLabel {
    private string _CostItemId;
    private int _CostAmount;
    private CozmoText _CostLabel;
    private SparkCostTint _CostTint;

    public CostLabel(string itemId, int costAmount, CozmoText costLabel, SparkCostTint costTint) {
      _CostItemId = itemId;
      _CostAmount = costAmount;
      _CostLabel = costLabel;
      _CostTint = costTint;

      _CostLabel.text = Localization.GetNumber(_CostAmount);

      Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      Inventory.ItemAdded += HandleItemValueChanged;
      Inventory.ItemRemoved += HandleItemValueChanged;
      Inventory.ItemCountSet += HandleItemValueChanged;

      SetSparkTextColor(playerInventory.GetItemAmount(_CostItemId));
    }

    public void DeregisterEvents() {
      Inventory.ItemAdded -= HandleItemValueChanged;
      Inventory.ItemRemoved -= HandleItemValueChanged;
      Inventory.ItemCountSet -= HandleItemValueChanged;
    }

    private void HandleItemValueChanged(string itemId, int delta, int newCount) {
      if (itemId == _CostItemId) {
        SetSparkTextColor(newCount);
      }
    }

    private void SetSparkTextColor(int itemCount) {
      if (itemCount >= _CostAmount) {
        _CostLabel.color = _CostTint.CanAffordColor;
      }
      else {
        _CostLabel.color = _CostTint.CannotAffordColor;
      }
    }
  }
}