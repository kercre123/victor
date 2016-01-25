using UnityEngine;
using System.Collections;
using Cozmo.UI;
using DG.Tweening;

namespace Cozmo {
  namespace MinigameWidgets {
    public class HowToPlayView : BaseView {

      [SerializeField]
      private CanvasGroup _AlphaController;

      [SerializeField]
      private Transform _ContentsContainer;

      public void Initialize(GameObject contentsPrefab) {
        if (contentsPrefab != null) {
          UIManager.CreateUIElement(contentsPrefab, _ContentsContainer);
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