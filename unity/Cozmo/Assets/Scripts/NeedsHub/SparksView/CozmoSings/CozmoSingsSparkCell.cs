using System;
using Cozmo.Needs.Sparks.UI;
using Cozmo.UI;
using UnityEngine;

namespace Cozmo.Needs.Sparks.UI.CozmoSings {
  public class CozmoSingsSparkCell : SparkCell {

    public override void Initialize(UnlockableInfo unlockInfo, string dasEventDialogName) {
      // Change from base: removed all sparks inv./display related stuff
      _SparksButton.onClick.AddListener(HandleTappedUnlocked);

      _TrickIcon.sprite = unlockInfo.CoreUpgradeIcon;
      _TrickTitleText.text = Localization.Get(unlockInfo.TitleKey);

      _UnlockInfo = unlockInfo;
    }

    protected override void HandleTappedUnlocked() {
      // Change from base: pop up Sings detail modal
      UIManager.OpenModal(_SparksDetailModalPrefab, new ModalPriorityData(), (obj) => {
        CozmoSingsDetailModal sparksDetailModal = (CozmoSingsDetailModal)obj;
        sparksDetailModal.InitializeSparksDetailModal(_UnlockInfo, isEngineDriven: false);
      });
    }
  }
}

