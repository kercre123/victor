using UnityEngine;
using DG.Tweening;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class CozmoStatusWidget : MinigameWidget {

      private const float kAnimXOffset = 600.0f;
      private const float kAnimYOffset = -300.0f;
      private const float kAnimDur = 0.25f;

      [SerializeField]
      private SegmentedBar _AttemptsDisplay;

      public int MaxAttempts {
        set {
          _AttemptsDisplay.SetMaximumSegments(value);
        }
      }

      public int AttemptsLeft {
        set {
          _AttemptsDisplay.SetCurrentNumSegments(value);
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