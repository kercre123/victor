using UnityEngine;
using Cozmo.UI;
using DG.Tweening;

namespace Cozmo.Challenge.DroneMode {
  public class DroneModeHowToPlayModal : BaseModal {

    [SerializeField]
    private Animator[] _HowToPlaySlideAnimators;

    [SerializeField]
    private int _SlideIndexToAlwaysAnimate = 0;

    [SerializeField]
    private CozmoButtonLegacy _PreviousSlideButton;

    [SerializeField]
    private CozmoButtonLegacy _NextSlideButton;

    private int _CurrentSlideIndex;
    private bool _IsPlayingAnimations = false;
    private int _MostProgressedSlide = -1;

    private void Awake() {
      _PreviousSlideButton.Initialize(HandlePreviousSlideButtonPressed, "drone_mode_how_to_play_previous_slide_button", this.DASEventDialogName);
      _NextSlideButton.Initialize(HandleNextSlideButtonPressed, "drone_mode_how_to_play_next_slide_button", this.DASEventDialogName);
      SetCurrentSlide(0);
      _AlphaController.alpha = 0;
    }

    private void OnDestroy() {
      if (_IsPlayingAnimations) {
        for (int i = 0; i < _HowToPlaySlideAnimators.Length; i++) {
          if (_HowToPlaySlideAnimators[i] != null) {
            _HowToPlaySlideAnimators[i].StopListenForAnimationEnd(HandleSlideAnimationEnd);
          }
        }
      }
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

    public void Initialize(bool showCornerCloseButton, bool playAnimations) {
      if (!showCornerCloseButton && _OptionalCloseDialogButton != null) {
        _OptionalCloseDialogButton.gameObject.SetActive(showCornerCloseButton);
      }

      _IsPlayingAnimations = playAnimations;
      if (_IsPlayingAnimations) {
        _NextSlideButton.Interactable = false;
        _PreviousSlideButton.Interactable = false;
        _HowToPlaySlideAnimators[0].ListenForAnimationEnd(HandleSlideAnimationEnd);
      }
      else {
        for (int i = 0; i < _HowToPlaySlideAnimators.Length; i++) {
          if (i != _SlideIndexToAlwaysAnimate) {
            _HowToPlaySlideAnimators[i].enabled = false;
          }
        }
      }
    }

    private void HandleSlideAnimationEnd(AnimatorStateInfo animationInfo) {
      _NextSlideButton.Interactable = true;
      _PreviousSlideButton.Interactable = !IsFirstSlide();
    }

    private void HandlePreviousSlideButtonPressed() {
      SetCurrentSlide(_CurrentSlideIndex - 1);
    }

    private void HandleNextSlideButtonPressed() {
      if (!IsLastSlide()) {
        if (_IsPlayingAnimations && (_CurrentSlideIndex != _SlideIndexToAlwaysAnimate) && (_CurrentSlideIndex == _MostProgressedSlide)) {
          _HowToPlaySlideAnimators[_CurrentSlideIndex].enabled = false;
        }

        int priorMostProgressedSlide = _MostProgressedSlide;

        SetCurrentSlide(_CurrentSlideIndex + 1);

        if (_IsPlayingAnimations) {
          if (_CurrentSlideIndex == _MostProgressedSlide && priorMostProgressedSlide != _MostProgressedSlide) {
            _HowToPlaySlideAnimators[_CurrentSlideIndex].ListenForAnimationEnd(HandleSlideAnimationEnd);
            _NextSlideButton.Interactable = false;
            _PreviousSlideButton.Interactable = false;
          }
          else {
            _NextSlideButton.Interactable = true;
            _PreviousSlideButton.Interactable = true;
          }
        }
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

      if (_CurrentSlideIndex > _MostProgressedSlide) {
        _MostProgressedSlide = _CurrentSlideIndex;
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