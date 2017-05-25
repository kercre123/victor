using System.Collections.Generic;
using UnityEngine;
using Cozmo.UI;

namespace Cozmo.Needs.Sparks.UI {
  public class SparkCell : MonoBehaviour {

    private const float kLockedAlpha = 0.5f;

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

    private ModalPriorityData _SparksDetailModalPriorityData = new ModalPriorityData(ModalPriorityLayer.VeryLow,
                                                                                     1,
                                                                                     LowPriorityModalAction.CancelSelf,
                                                                                     HighPriorityModalAction.Stack);

    private UnlockableInfo _UnlockInfo;

    public void Initialize(UnlockableInfo unlockInfo) {

      if (unlockInfo.ComingSoon) {
        _SparksButton.onClick.AddListener(HandleTappedComingSoon);
        _TrickIcon.color = new Color(_TrickIcon.color.r, _TrickIcon.color.g, _TrickIcon.color.b, kLockedAlpha);
        _SparkCostContainer.gameObject.SetActive(false);
      }
      else if (UnlockablesManager.Instance.IsUnlocked(unlockInfo.Id.Value)) {
        _SparksButton.onClick.AddListener(HandleTappedUnlocked);
      }
      else {
        _SparksButton.onClick.AddListener(HandleTappedLocked);
        _TrickIcon.color = new Color(_TrickIcon.color.r, _TrickIcon.color.g, _TrickIcon.color.b, kLockedAlpha);
      }

      _TrickIcon.sprite = unlockInfo.CoreUpgradeIcon;
      _TrickTitleText.text = Localization.Get(unlockInfo.TitleKey);
      _UnlockInfo = unlockInfo;

      // Always request a flat cost for performing a trick
      _SparkCountText.text = Localization.GetNumber(unlockInfo.RequestTrickCostAmountNeededMin);
    }

    private void HandleTappedComingSoon() {
      var cozmoNotReadyData = new AlertModalData("coming_soon_sparks_cell",
                                                 LocalizationKeys.kUnlockableComingSoonTitle,
                                                 LocalizationKeys.kUnlockableComingSoonDescription,
                                           new AlertModalButtonData("text_close_button", LocalizationKeys.kButtonClose));

      UIManager.OpenAlert(cozmoNotReadyData, new ModalPriorityData());
    }

    private void HandleTappedUnlocked() {
      // pop up sparks modal
      UIManager.OpenModal(_SparksDetailModalPrefab, _SparksDetailModalPriorityData, (obj) => {
        SparksDetailModal sparksDetailModal = (SparksDetailModal)obj;
        sparksDetailModal.InitializeSparksDetailModal(_UnlockInfo, false);
      });
    }

    private void HandleTappedLocked() {
      // pop up sparks modal with locked text info
      UIManager.OpenModal(_SparksDetailModalPrefab, _SparksDetailModalPriorityData, (obj) => {
        SparksDetailModal sparksDetailModal = (SparksDetailModal)obj;
        sparksDetailModal.InitializeSparksDetailModal(_UnlockInfo, true);
      });
    }

  }
}