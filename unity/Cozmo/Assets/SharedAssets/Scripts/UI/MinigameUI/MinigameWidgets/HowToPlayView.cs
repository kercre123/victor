using UnityEngine;
using System.Collections;
using Cozmo.UI;
using DG.Tweening;

namespace Cozmo {
  namespace MinigameWidgets {
    /// <summary>
    /// How to play view. Shows either a text field or a 
    /// custom prefab within a scroll rect.
    /// </summary>
    public class HowToPlayView : BaseView {

      [SerializeField]
      private CanvasGroup _AlphaController;

      [SerializeField]
      private Transform _ContentsContainer;

      [SerializeField]
      private Anki.UI.AnkiTextLabel _ContentsTextLabel;

      public void Initialize(string contentsLocKey) {
        if (!string.IsNullOrEmpty(contentsLocKey)) {
          _ContentsTextLabel.gameObject.SetActive(true);
          _ContentsTextLabel.text = Localization.Get(contentsLocKey);
          _ContentsContainer.gameObject.SetActive(false);
        }
        else {
          _ContentsTextLabel.gameObject.SetActive(false);
        }
      }

      public void Initialize(GameObject contentsPrefab) {
        if (contentsPrefab != null) {
          _ContentsContainer.gameObject.SetActive(true);
          UIManager.CreateUIElement(contentsPrefab, _ContentsContainer);
          _ContentsTextLabel.gameObject.SetActive(false);
        }
        else {
          _ContentsContainer.gameObject.SetActive(false);
        }
      }

      #region Base View

      protected override void CleanUp() {
        
      }

      protected override void ConstructOpenAnimation(Sequence openAnimation) {
        openAnimation.Append(transform.DOLocalMoveY(
          50, 0.15f).From().SetEase(Ease.OutQuad).SetRelative());
        if (_AlphaController != null) {
          _AlphaController.alpha = 0;
          openAnimation.Join(_AlphaController.DOFade(1, 0.25f).SetEase(Ease.OutQuad));
        }
      }

      protected override void ConstructCloseAnimation(Sequence closeAnimation) {
        closeAnimation.Append(transform.DOLocalMoveY(
          -50, 0.15f).SetEase(Ease.OutQuad).SetRelative());
        if (_AlphaController != null) {
          closeAnimation.Join(_AlphaController.DOFade(0, 0.25f));
        }
      }

      #endregion
    }
  }
}