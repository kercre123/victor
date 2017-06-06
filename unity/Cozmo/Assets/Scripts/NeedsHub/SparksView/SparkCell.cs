using System.Collections.Generic;
using UnityEngine;
using Cozmo.UI;

namespace Cozmo.Needs.Sparks.UI {
  public class SparkCell : MonoBehaviour {

    private const float kComingSoonAlpha = 0.5f;

    [SerializeField]
    private CozmoImage _TrickIcon;

    [SerializeField]
    private CozmoText _TrickTitleText;

    [SerializeField]
    private CozmoButton _SparksButton;

    [SerializeField]
    private CozmoText _SparkCountText;

    [SerializeField]
    private GameObject _SparkCostContainer;

    [SerializeField]
    private SparksDetailModal _SparksDetailModalPrefab;

    private UnlockableInfo _UnlockInfo;
    private CostLabel _CostLabelHelper;

    public void Initialize(UnlockableInfo unlockInfo, string dasEventDialogName) {
      _SparksButton.Initialize(null, "spark_cell_" + unlockInfo.DASName, dasEventDialogName);
      if (unlockInfo.ComingSoon) {
        _SparksButton.onClick.AddListener(HandleTappedComingSoon);
        _TrickIcon.color = new Color(_TrickIcon.color.r, _TrickIcon.color.g, _TrickIcon.color.b, kComingSoonAlpha);
        _SparkCostContainer.gameObject.SetActive(false);
      }
      else {
        _SparksButton.onClick.AddListener(HandleTappedUnlocked);
      }

      _TrickIcon.sprite = unlockInfo.CoreUpgradeIcon;
      _TrickTitleText.text = Localization.Get(unlockInfo.TitleKey);

      _UnlockInfo = unlockInfo;

      _CostLabelHelper = new CostLabel(unlockInfo.RequestTrickCostItemId,
                                       unlockInfo.RequestTrickCostAmount,
                                       _SparkCountText,
                                       UIColorPalette.GeneralSparkTintColor);
    }

    private void OnDestroy() {
      _CostLabelHelper.DeregisterEvents();
    }

    private void HandleTappedComingSoon() {
      var cozmoNotReadyData = new AlertModalData("coming_soon_sparks_cell",
                                                 LocalizationKeys.kUnlockableComingSoonTitle,
                                                 LocalizationKeys.kUnlockableComingSoonDescription,
                                           new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose));

      ModalPriorityData comingSoonPriority = new ModalPriorityData(ModalPriorityLayer.VeryLow,
                                         0,
                                         LowPriorityModalAction.CancelSelf,
                                         HighPriorityModalAction.Stack);

      UIManager.OpenAlert(cozmoNotReadyData, comingSoonPriority);
    }

    private void HandleTappedUnlocked() {
      // pop up sparks modal
      UIManager.OpenModal(_SparksDetailModalPrefab, new ModalPriorityData(), (obj) => {
        SparksDetailModal sparksDetailModal = (SparksDetailModal)obj;
        sparksDetailModal.InitializeSparksDetailModal(_UnlockInfo);
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
      playerInventory.ItemAdded += HandleItemValueChanged;
      playerInventory.ItemRemoved += HandleItemValueChanged;
      playerInventory.ItemCountSet += HandleItemValueChanged;

      SetSparkTextColor(playerInventory.GetItemAmount(_CostItemId));
    }

    public void DeregisterEvents() {
      Inventory playerInventory = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.Inventory;
      playerInventory.ItemAdded -= HandleItemValueChanged;
      playerInventory.ItemRemoved -= HandleItemValueChanged;
      playerInventory.ItemCountSet -= HandleItemValueChanged;
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