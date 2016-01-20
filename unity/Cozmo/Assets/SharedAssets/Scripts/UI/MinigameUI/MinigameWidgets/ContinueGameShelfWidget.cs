using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;
using Anki.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class ContinueGameShelfWidget : MonoBehaviour, IMinigameWidget {

      public delegate void ContinueButtonClickHandler();

      [SerializeField]
      private AnkiButton _ContinueButton;

      [SerializeField]
      private AnkiTextLabel _ShelfTextLabel;

      private bool _ShouldButtonBeInteractive = false;
      private ContinueButtonClickHandler _OnClickCallback;

      private void Start() {
        _ContinueButton.onClick.AddListener(HandleContinueButtonClicked);
      }

      public void SetShelfText(string text) {
        _ShelfTextLabel.text = text;
      }

      public void SetButtonText(string text) {
        _ContinueButton.Text = text;
      }

      public void SetButtonListener(ContinueButtonClickHandler buttonClickHandler) {
        _OnClickCallback = buttonClickHandler;
      }

      private void HandleContinueButtonClicked() {
        if (_OnClickCallback != null) {
          _OnClickCallback();
        }
      }

      public void SetButtonInteractivity(bool enableButton) {
        _ShouldButtonBeInteractive = enableButton;
        _ContinueButton.Interactable = _ShouldButtonBeInteractive;
      }

      #region IMinigameWidget

      public void DestroyWidgetImmediately() {
        Destroy(gameObject);
      }

      // TODO: Don't hardcode this
      public Sequence OpenAnimationSequence() {
        Sequence open = DOTween.Sequence();
        open.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x, 
          this.transform.localPosition.y - 300, this.transform.localPosition.z),
          0.25f).From().SetEase(Ease.OutQuad));
        return open;
      }

      // TODO: Don't hardcode this
      public Sequence CloseAnimationSequence() {
        Sequence close = DOTween.Sequence();
        close.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x, 
          this.transform.localPosition.y - 300, this.transform.localPosition.z),
          0.25f).SetEase(Ease.OutQuad));
        return close;
      }

      public void EnableInteractivity() {
        _ContinueButton.Interactable = _ShouldButtonBeInteractive;
      }

      public void DisableInteractivity() {
        _ContinueButton.Interactable = false;
        
      }

      #endregion
    }
  }
}