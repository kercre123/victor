using System;
using Cozmo.Needs.Sparks.UI;
using Cozmo.UI;
using UnityEngine;

namespace Cozmo.Needs.Sparks.UI.CozmoSings {
  public class CozmoSingsDetailModal : SparksDetailModal {

    [SerializeField]
    private CozmoSingsSongListModal _SongListModal;

    public override void InitializeSparksDetailModal(UnlockableInfo unlockInfo, bool isEngineDriven) {
      _UnlockInfo = unlockInfo;
      _IsEngineDrivenTrick = isEngineDriven;

      InitializeUnlockInfo();

      // Initialize song list modal after being opened
      System.Action<BaseModal> songListModalOpened = (modal) => {
        CozmoSingsSongListModal castedModal = modal.GetComponent<CozmoSingsSongListModal>();
        castedModal.Initialize();
      };

      _SparkButton.Initialize(() => {
        UIManager.OpenModal(_SongListModal,
                          SettingsModal.SettingsSubModalPriorityData(), songListModalOpened);
      }, "song_list_modal_opened", this.DASEventDialogName);
    }

    protected override void InitializeUnlockInfo() {
      this.DASEventDialogName = this.DASEventDialogName + "_" + _UnlockInfo.DASName;

      // Change from base: optional instead of required
      _CubesRequiredLabel.text = Localization.GetWithArgs(LocalizationKeys.kCoreUpgradeDetailsDialogCubesOptional,
        _UnlockInfo.CubesRequired,
        ItemDataConfig.GetCubeData().GetAmountName(_UnlockInfo.CubesRequired));

      _ButtonPromptTitle.text = Localization.Get(LocalizationKeys.kUnlockableCozmoSingsButtonPromptTitle);
      _ButtonPromptDescription.text = Localization.Get(LocalizationKeys.kUnlockableCozmoSingsButtonPromptDescription);
    }
  }
}
