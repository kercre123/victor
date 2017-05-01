using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;

namespace Cozmo.UI {
  public class BackgroundColorController : MonoBehaviour {
    public enum BackgroundColor {
      Yellow,
      Bone,
      Beige,
      TintMe
    }

    // INGO TODO: Replace all this with a shader
    // See ticket https://ankiinc.atlassian.net/browse/COZMO-2516
    [SerializeField]
    private Image _FromBackgroundImage;

    [SerializeField]
    private Image _ToBackgroundImage;

    [SerializeField]
    private Sprite _YellowBGGradient;

    [SerializeField]
    private Sprite _BoneBGGradient;

    [SerializeField]
    private Sprite _BeigeBGGradient;

    [SerializeField]
    private Sprite _TintMeBGGradient;

    [SerializeField]
    private float _BackgroundColorTransition_sec;

    private Tween _TransitionTween;
    private Color _TargetTintColor;

    private void OnDestroy() {
      CleanUpTransitionTween();
    }

    private void CleanUpTransitionTween() {
      if (_TransitionTween != null) {
        _TransitionTween.Kill();
        _TransitionTween = null;
      }
    }

    public void SetBackgroundColor(BackgroundColor baseColor) {
      SetBackgroundColor(baseColor, Color.white);
    }

    public void SetBackgroundColor(BackgroundColor baseColor, Color tintColor) {
      if (this != null) {
        CleanUpTransitionTween();

        if (_FromBackgroundImage != null && _FromBackgroundImage.gameObject != null) {
          _FromBackgroundImage.gameObject.SetActive(true);
          if (_ToBackgroundImage != null) {
            _FromBackgroundImage.sprite = _ToBackgroundImage.sprite;
          }
          else {
            DAS.Error("BackgroundColorController.SetBackgroundColor", "ToBackgroundImage is NULL");
          }
          _FromBackgroundImage.color = _TargetTintColor;
        }
        else {
          DAS.Error("BackgroundColorController.SetBackgroundColor", "FromBackgroundImage is NULL");
        }
        if (_ToBackgroundImage != null) {
          Sprite newBackgroundSprite = GetSpriteBasedOnColor(baseColor);
          _ToBackgroundImage.sprite = newBackgroundSprite;

          Color transparentTint = new Color(tintColor.r, tintColor.g, tintColor.b, 0);
          _ToBackgroundImage.color = transparentTint;

          _TargetTintColor = tintColor;
          _TransitionTween = _ToBackgroundImage.DOFade(1, _BackgroundColorTransition_sec)
                                               .SetEase(UIDefaultTransitionSettings.Instance.FadeInEasing)
                                               .OnComplete(() => {
                                                 if (_FromBackgroundImage != null) {
                                                   _FromBackgroundImage.gameObject.SetActive(false);
                                                 }
                                               });
          _TransitionTween.Play();
        }
        else {
          DAS.Error("BackgroundColorController.SetBackgroundColor", "ToBackgroundImage is NULL");
        }
      }
    }

    private Sprite GetSpriteBasedOnColor(BackgroundColor desiredSprite) {
      Sprite backgroundSprite = null;
      switch (desiredSprite) {
      case BackgroundColor.Beige:
        backgroundSprite = _BeigeBGGradient;
        break;
      case BackgroundColor.Bone:
        backgroundSprite = _BoneBGGradient;
        break;
      case BackgroundColor.Yellow:
        backgroundSprite = _YellowBGGradient;
        break;
      case BackgroundColor.TintMe:
        backgroundSprite = _TintMeBGGradient;
        break;
      }
      return backgroundSprite;
    }
  }
}