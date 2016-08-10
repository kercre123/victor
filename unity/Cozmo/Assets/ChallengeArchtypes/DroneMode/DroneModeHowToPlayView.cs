using UnityEngine;
using Cozmo.UI;
using DG.Tweening;

namespace Cozmo.Minigame.DroneMode {
  public class DroneModeHowToPlayView : BaseView {

    [SerializeField]
    private CanvasGroup _AlphaController;

    protected override void ConstructOpenAnimation(Sequence openAnimation) {
      float fadeInSeconds = UIDefaultTransitionSettings.Instance.FadeInTransitionDurationSeconds;
      Ease fadeInEasing = UIDefaultTransitionSettings.Instance.FadeInEasing;
      openAnimation.Append(_AlphaController.DOFade(1f, fadeInSeconds).SetEase(fadeInEasing));
    }

    protected override void ConstructCloseAnimation(Sequence closeAnimation) {
      float fadeOutSeconds = UIDefaultTransitionSettings.Instance.FadeOutTransitionDurationSeconds;
      Ease fadeOutEasing = UIDefaultTransitionSettings.Instance.FadeOutEasing;
      closeAnimation.Append(_AlphaController.DOFade(0f, fadeOutSeconds).SetEase(fadeOutEasing));
    }
  }
}