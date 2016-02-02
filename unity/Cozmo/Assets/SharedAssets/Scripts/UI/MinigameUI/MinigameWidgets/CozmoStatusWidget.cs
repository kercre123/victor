using UnityEngine;
using DG.Tweening;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class CozmoStatusWidget : MonoBehaviour, IMinigameWidget {

      [SerializeField]
      private SegmentedBar _AttemptsDisplay;

      // TODO: Add handling of cozmo's face view here

      public void SetMaxAttempts(int maximumAttempts) {
        _AttemptsDisplay.SetMaximumSegments(maximumAttempts);
      }

      public void SetAttemptsLeft(int attemptsLeft) {
        _AttemptsDisplay.SetCurrentNumSegments(attemptsLeft);
      }

      #region IMinigameWidget

      public void DestroyWidgetImmediately() {
        Destroy(gameObject);
      }

      // TODO: Don't hardcode this
      public Sequence OpenAnimationSequence() {
        Sequence open = DOTween.Sequence();
        open.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x + 600, 
          this.transform.localPosition.y - 300, this.transform.localPosition.z),
          0.25f).From().SetEase(Ease.OutQuad));
        return open;
      }

      // TODO: Don't hardcode this
      public Sequence CloseAnimationSequence() {
        Sequence close = DOTween.Sequence();
        close.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x + 600, 
          this.transform.localPosition.y - 300, this.transform.localPosition.z),
          0.25f).SetEase(Ease.OutQuad));
        return close;
      }

      #endregion
    }
  }
}