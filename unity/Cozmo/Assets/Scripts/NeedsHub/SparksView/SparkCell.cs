using System.Collections.Generic;
using UnityEngine;
using Cozmo.UI;

namespace Cozmo.Needs.Sparks.UI {
  public class SparkCell : MonoBehaviour {

    private const float kLockedAlpha = 0.5f;

    [SerializeField]
    private CozmoImage _SparkIcon;

    [SerializeField]
    private CozmoText _SparkTitleText;

    [SerializeField]
    private CozmoButton _SparksButton;

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
        _SparkIcon.color = new Color(_SparkIcon.color.r, _SparkIcon.color.g, _SparkIcon.color.b, kLockedAlpha);
      }
      else if (UnlockablesManager.Instance.IsUnlocked(unlockInfo.Id.Value)) {
        _SparksButton.onClick.AddListener(HandleTappedUnlocked);
      }
      else {
        _SparksButton.onClick.AddListener(HandleTappedLocked);
        _SparkIcon.color = new Color(_SparkIcon.color.r, _SparkIcon.color.g, _SparkIcon.color.b, kLockedAlpha);
      }

      _SparkIcon.sprite = unlockInfo.CoreUpgradeIcon;
      _SparkTitleText.text = Localization.Get(unlockInfo.TitleKey);
      _UnlockInfo = unlockInfo;

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