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
    public class HowToPlayView : BaseModal {

      [SerializeField]
      private Transform _ContentsContainer;

      [SerializeField]
      private Anki.UI.AnkiTextLabel _ContentsTextLabel;

      public void Initialize(string contentsLocKey, GameObject contentsPrefab) {
        if (!string.IsNullOrEmpty(contentsLocKey)) {
          _ContentsTextLabel.text = Localization.Get(contentsLocKey);
        }
        if (contentsPrefab != null) {
          UIManager.CreateUIElement(contentsPrefab, _ContentsContainer);
        }
      }

      #region Base View

      protected override void CleanUp() {

      }

      protected override void ConstructOpenAnimation(Sequence openAnimation) {
        openAnimation.Append(transform.DOLocalMoveX(
          -100, 0.25f).From().SetEase(Ease.OutQuad).SetRelative());
        if (_AlphaController != null) {
          _AlphaController.alpha = 0;
          openAnimation.Join(_AlphaController.DOFade(1, 0.25f).SetEase(Ease.OutQuad));
        }
      }

      protected override void ConstructCloseAnimation(Sequence closeAnimation) {
        closeAnimation.Append(transform.DOLocalMoveX(
          -100, 0.25f).SetEase(Ease.OutQuad).SetRelative());
        if (_AlphaController != null) {
          closeAnimation.Join(_AlphaController.DOFade(0, 0.25f));
        }
      }

      #endregion
    }
  }
}