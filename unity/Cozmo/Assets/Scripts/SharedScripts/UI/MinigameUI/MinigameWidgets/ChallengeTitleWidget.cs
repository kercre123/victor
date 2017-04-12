using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;

namespace Cozmo {
  namespace MinigameWidgets {
    public class ChallengeTitleWidget : MinigameWidget {

      private const float kAnimXOffset = 0f;
      private const float kAnimYOffset = 400.0f;
      private const float kAnimDur = 0.25f;

      [SerializeField]
      private Anki.UI.AnkiTextLegacy _ChallengeTitleLabel;

      [SerializeField]
      private Anki.UI.AnkiTextLegacy _ChallengeSubtitleLabel;

      public string Text {
        set {
          _ChallengeTitleLabel.text = value;
        }
      }

      public string SubtitleText {
        set {
          _ChallengeSubtitleLabel.text = value;
        }
      }

      public Color TitleTextColor {
        set {
          _ChallengeTitleLabel.color = value;
        }
      }

      public Color SubtitleTextColor {
        set {
          _ChallengeSubtitleLabel.color = value;
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