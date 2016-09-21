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
      CleanUpTransitionTween();

      _FromBackgroundImage.gameObject.SetActive(true);
      _FromBackgroundImage.sprite = _ToBackgroundImage.sprite;
      _FromBackgroundImage.color = _TargetTintColor;

      Sprite newBackgroundSprite = GetSpriteBasedOnColor(baseColor);
      _ToBackgroundImage.sprite = newBackgroundSprite;

      Color transparentTint = new Color(tintColor.r, tintColor.g, tintColor.b, 0);
      _ToBackgroundImage.color = transparentTint;

      _TargetTintColor = tintColor;
      _TransitionTween = _ToBackgroundImage.DOFade(1, _BackgroundColorTransition_sec)
                                           .SetEase(UIDefaultTransitionSettings.Instance.FadeInEasing)
                                           .OnComplete(() => _FromBackgroundImage.gameObject.SetActive(false));
      _TransitionTween.Play();
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