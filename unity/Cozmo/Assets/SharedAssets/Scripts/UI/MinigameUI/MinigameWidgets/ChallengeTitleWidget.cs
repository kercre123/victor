using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;

namespace Cozmo {
  namespace MinigameWidgets {
    public class ChallengeTitleWidget : MinigameWidget {

      private const float kAnimXOffset = 600.0f;
      private const float kAnimYOffset = 300.0f;
      private const float kAnimDur = 0.25f;

      [SerializeField]
      private Cozmo.UI.IconProxy _ChallangeIcon;

      [SerializeField]
      private Anki.UI.AnkiTextLabel _ChallengeTitleLabel;

      public string Text {
        set {
          _ChallengeTitleLabel.text = value;
        }
      }

      public Sprite Icon {
        set {
          _ChallangeIcon.SetIcon(value);
        }
      }

      #region IMinigameWidget

      public override void DestroyWidgetImmediately() {
        Destroy(gameObject);
      }

      public override Sequence CreateOpenAnimSequence() {
        return CreateOpenAnimSequence(kAnimXOffset, kAnimYOffset, kAnimDur);
      }

      public override Sequence CreateCloseAnimSequence() {
        return CreateCloseAnimSequence(kAnimXOffset, kAnimYOffset, kAnimDur);
      }

      #endregion
    }
  }
}