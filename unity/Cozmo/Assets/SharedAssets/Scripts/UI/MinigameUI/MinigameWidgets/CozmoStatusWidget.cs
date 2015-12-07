using UnityEngine;
using DG.Tweening;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class CozmoStatusWidget : MonoBehaviour, IMinigameWidget {

      [SerializeField]
      private SegmentedBarWidget _AttemptsDisplay;

      // TODO: Add handling of cozmo's face view here

      public void SetMaxAttempts(int maximumAttempts) {
        _AttemptsDisplay.SetMaximumSegments(maximumAttempts);
      }

      public void SetAttemptsLeft(int attemptsLeft) {
        _AttemptsDisplay.SetCurrentNumSegments(attemptsLeft);
      }

      public void DestroyWidgetImmediately() {
        Destroy(gameObject);
      }

      public Sequence OpenAnimationSequence() {
        // TODO
        return null;
      }

      public Sequence CloseAnimationSequence() {
        // TODO
        return null;
      }

      public void EnableInteractivity() {
        // Nothing interactive to enable
      }

      public void DisableInteractivity() {
        // Nothing interactive to disable
      }
    }
  }
}