using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Cozmo.Challenge;
using Cozmo.UI;

namespace Cozmo.Needs.Sparks.UI {
  public class GameCell : MonoBehaviour {

    [SerializeField]
    private CozmoImage _GameIcon;

    [SerializeField]
    private CozmoText _GameTitle;

    [SerializeField]
    private CozmoButton _GameButton;

    [SerializeField]
    private CozmoText _SparkCountText;

    [SerializeField]
    private SparksDetailModal _SparksDetailModalPrefab;

    private ChallengeManager.ChallengeStatePacket _ChallengePacket;
    private CostLabel _CostLabelHelper;

    public void Initialize(ChallengeManager.ChallengeStatePacket challengePacket, string dasEventDialogName) {
      _ChallengePacket = challengePacket;
      _GameIcon.sprite = challengePacket.Data.ChallengeIcon;
      _GameTitle.text = Localization.Get(challengePacket.Data.ChallengeTitleLocKey);
      _GameButton.Initialize(HandleOpenGameDetailsPressed, challengePacket.Data.DASName, dasEventDialogName);

      UnlockableInfo unlockInfo = UnlockablesManager.Instance.GetUnlockableInfo(challengePacket.Data.UnlockId.Value);

      _CostLabelHelper = new CostLabel(unlockInfo.RequestTrickCostItemId,
                                       unlockInfo.RequestTrickCostAmount,
                                       _SparkCountText,
                                       UIColorPalette.GeneralSparkTintColor);
    }

    private void OnDestroy() {
      _CostLabelHelper.DeregisterEvents();
    }

    private void HandleOpenGameDetailsPressed() {
      UIManager.OpenModal(_SparksDetailModalPrefab, new ModalPriorityData(), (obj) => {
        SparksDetailModal sparksDetailModal = (SparksDetailModal)obj;
        sparksDetailModal.InitializeSparksDetailModal(_ChallengePacket);
      });
    }
  }
}