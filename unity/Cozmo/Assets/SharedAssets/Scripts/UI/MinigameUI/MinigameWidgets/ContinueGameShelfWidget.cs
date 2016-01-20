using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;

namespace Cozmo {
  namespace MinigameWidgets {
    public class ContinueGameShelfWidget : MonoBehaviour, IMinigameWidget {

      public delegate void ContinueButtonClickHandler();

      public void Initialize(string buttonText, string helpText, ContinueButtonClickHandler buttonCallback) {
      }

      public void SetButtonEnabled(bool enableButton) {
      }

      #region IMinigameWidget

      public void DestroyWidgetImmediately() {
        Destroy(gameObject);
      }

      // TODO: Don't hardcode this
      public Sequence OpenAnimationSequence() {
        Sequence open = DOTween.Sequence();
        open.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x, 
          this.transform.localPosition.y + 300, this.transform.localPosition.z),
          0.25f).From().SetEase(Ease.OutQuad));
        return open;
      }

      // TODO: Don't hardcode this
      public Sequence CloseAnimationSequence() {
        Sequence close = DOTween.Sequence();
        close.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x, 
          this.transform.localPosition.y + 300, this.transform.localPosition.z),
          0.25f).SetEase(Ease.OutQuad));
        return close;
      }

      public void EnableInteractivity() {
        // Nothing interactive to enable
      }

      public void DisableInteractivity() {
        // Nothing interactive to disable
      }

      #endregion
    }
  }
}