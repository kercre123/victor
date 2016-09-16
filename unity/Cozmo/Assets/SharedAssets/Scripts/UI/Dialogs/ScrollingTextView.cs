using Anki.UI;
using DG.Tweening;
using UnityEngine;

namespace Cozmo.UI {
  public class ScrollingTextView : BaseView {

    [SerializeField]
    private CanvasGroup _AlphaController;

    [SerializeField]
    private CanvasGroup _ScrollRectCanvasGroup;

    [SerializeField]
    private AnkiTextLabel _TitleTextLabel;

    [SerializeField]
    private AnkiInfiniteScrollView _DescriptionTextLabel;

    public void Initialize(string titleText, string descriptionText) {
      _TitleTextLabel.text = titleText;
      _DescriptionTextLabel.SetString(descriptionText);

      _AlphaController.interactable = true;
      _AlphaController.blocksRaycasts = false;

      _ScrollRectCanvasGroup.interactable = true;
      _ScrollRectCanvasGroup.blocksRaycasts = true;
      _ScrollRectCanvasGroup.ignoreParentGroups = true;
    }

    protected override void CleanUp() {

    }

    protected override void ConstructOpenAnimation(Sequence openAnimation) {
      UIDefaultTransitionSettings settings = UIDefaultTransitionSettings.Instance;
      openAnimation.Append(transform.DOLocalMoveY(
        50, 0.15f).From().SetEase(settings.MoveOpenEase).SetRelative());
      if (_AlphaController != null) {
        _AlphaController.alpha = 0;
        openAnimation.Join(_AlphaController.DOFade(1, 0.25f).SetEase(settings.FadeInEasing));
      }
    }

    protected override void ConstructCloseAnimation(Sequence closeAnimation) {
      _ScrollRectCanvasGroup.ignoreParentGroups = false;
      UIDefaultTransitionSettings settings = UIDefaultTransitionSettings.Instance;
      closeAnimation.Append(transform.DOLocalMoveY(
        -50, 0.15f).SetEase(settings.MoveCloseEase).SetRelative());
      if (_AlphaController != null) {
        closeAnimation.Join(_AlphaController.DOFade(0, 0.25f).SetEase(settings.FadeOutEasing));
      }
    }
  }
}