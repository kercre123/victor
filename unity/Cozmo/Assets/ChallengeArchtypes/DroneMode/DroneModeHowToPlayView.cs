using UnityEngine;
using Cozmo.UI;
using DG.Tweening;

namespace Cozmo.Minigame.DroneMode {
  public class DroneModeHowToPlayView : BaseView {

    [SerializeField]
    private CanvasGroup _AlphaController;

    [SerializeField]
    private Animator[] _HowToPlaySlideAnimators;

    [SerializeField]
    private CozmoButton _PreviousSlideButton;

    [SerializeField]
    private CozmoButton _NextSlideButton;

    private int _CurrentSlideIndex;

    private void Awake() {
      _PreviousSlideButton.Initialize(HandlePreviousSlideButtonPressed, "drone_mode_how_to_play_previous_slide_button", this.DASEventViewName);
      _NextSlideButton.Initialize(HandleNextSlideButtonPressed, "drone_mode_how_to_play_next_slide_button", this.DASEventViewName);
      SetCurrentSlide(0);
    }

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

    public void ShowCloseButton(bool showCornerCloseButton) {
      if (!showCornerCloseButton && _OptionalCloseDialogButton != null) {
        _OptionalCloseDialogButton.gameObject.SetActive(showCornerCloseButton);
      }
    }

    private void HandlePreviousSlideButtonPressed() {
      SetCurrentSlide(_CurrentSlideIndex - 1);
    }

    private void HandleNextSlideButtonPressed() {
      if (!IsLastSlide()) {
        SetCurrentSlide(_CurrentSlideIndex + 1);
      }
      else {
        HandleUserClose();
      }
    }

    private void SetCurrentSlide(int newSlideIndex) {
      _CurrentSlideIndex = Mathf.Clamp(newSlideIndex, 0, _HowToPlaySlideAnimators.Length);

      _PreviousSlideButton.Interactable = !IsFirstSlide();

      if (_CurrentSlideIndex == _HowToPlaySlideAnimators.Length - 1) {
        _NextSlideButton.Text = Localization.Get(LocalizationKeys.kButtonClose);
      }
      else {
        _NextSlideButton.Text = Localization.Get(LocalizationKeys.kButtonNext);
      }

      for (int i = 0; i < _HowToPlaySlideAnimators.Length; i++) {
        _HowToPlaySlideAnimators[i].gameObject.SetActive(i == _CurrentSlideIndex);
      }
    }

    private bool IsFirstSlide() {
      return (_CurrentSlideIndex == 0);
    }

    private bool IsLastSlide() {
      return (_CurrentSlideIndex == _HowToPlaySlideAnimators.Length - 1);
    }
  }
}