using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;
using Anki.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class ContinueGameButtonWidget : MinigameWidget {

      private const float kAnimXOffset = 0.0f;
      private const float kAnimYOffset = -300.0f;
      private const float kAnimDur = 0.25f;

      public delegate void ContinueButtonClickHandler();

      [SerializeField]
      private Cozmo.UI.CozmoButton _ContinueButton;

      [SerializeField]
      private AnkiTextLabel _ShelfTextLabel;

      private ContinueButtonClickHandler _OnClickCallback;

      public string DASEventViewController {
        get { return _ContinueButton.DASEventViewController; }
        set { _ContinueButton.DASEventViewController = value; }
      }

      private void OnDestroy() {
        _ContinueButton.onClick.RemoveListener(HandleContinueButtonClicked);
      }

      public void Initialize(ContinueGameButtonWidget.ContinueButtonClickHandler buttonClickHandler,
                             string buttonText, string shelfText, Color shelfTextColor, string dasButtonName, string dasViewControllerName) {
        _ContinueButton.onClick.RemoveAllListeners();
        _ContinueButton.Initialize(HandleContinueButtonClicked, dasButtonName, dasViewControllerName);
        _ContinueButton.Text = buttonText;
        _OnClickCallback = buttonClickHandler;
        SetShelfText(shelfText, shelfTextColor);
      }

      public void SetShelfText(string text, Color textColor) {
        if (_ShelfTextLabel != null) {
          _ShelfTextLabel.text = text;
          _ShelfTextLabel.color = textColor;
        }
      }

      private void HandleContinueButtonClicked() {
        if (_OnClickCallback != null) {
          _OnClickCallback();
        }
      }

      public void SetButtonInteractivity(bool enableButton) {
        _ContinueButton.Interactable = enableButton;
      }

      public void GrowShelfBackground() {
      }

      public void ShrinkShelfBackground() {
      }

      public void HideCaratOffscreen() {
      }

      public void MoveCarat(float xPos) {
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