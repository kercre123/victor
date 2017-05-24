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
    private SparksDetailModal _SparksDetailModalPrefab;

    public void Initialize(ChallengeManager.ChallengeStatePacket challengePacket) {
      _GameIcon.sprite = challengePacket.Data.ChallengeIcon;
      _GameTitle.text = Localization.Get(challengePacket.Data.ChallengeTitleLocKey);
      _GameButton.onClick.AddListener(() => {
        UIManager.OpenModal(_SparksDetailModalPrefab, new ModalPriorityData(), (obj) => {
          SparksDetailModal sparksDetailModal = (SparksDetailModal)obj;
          sparksDetailModal.InitializeSparksDetailModal(challengePacket, UnlockablesManager.Instance.IsUnlocked(challengePacket.Data.UnlockId.Value));
        });
      });
    }
  }
}