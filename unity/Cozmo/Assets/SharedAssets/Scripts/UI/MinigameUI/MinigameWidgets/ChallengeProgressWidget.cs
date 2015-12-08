using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using Anki.UI;
using DG.Tweening;

namespace Cozmo {
  namespace MinigameWidgets {
    public class ChallengeProgressWidget : MonoBehaviour, IMinigameWidget {
      [SerializeField]
      private ProgressBarWidget _ChallengeProgressBar;

      [SerializeField]
      private AnkiTextLabel _ProgressBarLabel;

      #region IMinigameWidget

      public void DestroyWidgetImmediately() {
        Destroy(gameObject);
      }

      // TODO: Don't hardcode this
      public Sequence OpenAnimationSequence() {
        Sequence open = DOTween.Sequence();
        open.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x - 600, 
          this.transform.localPosition.y - 300, this.transform.localPosition.z),
          0.25f).From().SetEase(Ease.OutQuad));
        return open;
      }

      // TODO: Don't hardcode this
      public Sequence CloseAnimationSequence() {
        Sequence close = DOTween.Sequence();
        close.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x - 600, 
          this.transform.localPosition.y - 300, this.transform.localPosition.z),
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

      public string ProgressBarLabelText {
        get {
          return _ProgressBarLabel.text;
        }
        set {
          _ProgressBarLabel.text = value;
        }
      }

      public void ResetProgress() {
        _ChallengeProgressBar.ResetProgress();
      }

      public void SetProgress(float progress) {
        _ChallengeProgressBar.SetProgress(progress);
      }
    }
  }
}