using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;
using Anki.UI;

namespace Cozmo {
  namespace MinigameWidgets {

    public class ShelfWidget : MinigameWidget {

      private const float kAnimXOffset = 0.0f;
      private const float kAnimYOffset = -476.0f;
      private const float kAnimDur = 0.25f;

      [SerializeField]
      private RectTransform _BackgroundImageContainer;

      [SerializeField]
      private Image _BackgroundImage;

      [SerializeField]
      private float _StartYLocalPos = -300f;

      [SerializeField]
      private float _GrowTweenDurationSeconds = 0.2f;

      private Sequence _BackgroundTween = null;
      private bool _IsBackgroundGrown = false;

      [SerializeField]
      private RectTransform _CaratContainer;

      [SerializeField]
      private float _CaratTweenDurationSeconds = 0.4f;

      [SerializeField]
      private float _CaratExitTweenDurationSeconds = 0.2f;

      [SerializeField]
      private float _CaratLeftOffscreenLocalXPos = 2900;

      [SerializeField]
      private float _CaratRightOffscreenLocalXPos = -200f;

      private Tweener _CaratTween = null;

      [SerializeField]
      private CanvasGroup _ContentContainer;

      [SerializeField]
      private float _ContentFadeTweenDurationSeconds = 0.2f;

      [SerializeField]
      private float _ContentTweenXOffset = 100f;

      private Sequence _ContentTween = null;
      private GameObject _ContentObject = null;

      [SerializeField]
      private RectTransform _BannerContainer;

      [SerializeField]
      private AnkiTextLabel _BannerTextLabel;

      [SerializeField]
      private float _BannerInOutAnimationDurationSeconds = 0.3f;

      [SerializeField]
      private float _BannerSlowAnimationDurationSeconds = .75f;

      [SerializeField]
      private float _BannerLeftOffscreenLocalXPos = 0;

      [SerializeField]
      private float _BannerRightOffscreenLocalXPos = 0f;

      [SerializeField]
      private float _BannerSlowDistance = 100f;

      private Sequence _BannerTween = null;

      private void Start() {
        transform.SetAsFirstSibling();
        _BannerContainer.gameObject.SetActive(false);
      }

      private void OnDestroy() {
        if (_ContentTween != null) {
          _ContentTween.Kill();
        }
        if (_BackgroundTween != null) {
          _BackgroundTween.Kill();
        }
        if (_CaratTween != null) {
          _CaratTween.Kill();
        }
        if (_BannerTween != null) {
          _BannerTween.Kill();
        }
      }

      public void GrowShelfBackground() {
        if (!_IsBackgroundGrown) {
          _IsBackgroundGrown = true;
          PlayBackgroundTween(0f, Ease.OutQuad);
        }
      }

      public void ShrinkShelfBackground() {
        if (_IsBackgroundGrown) {
          _IsBackgroundGrown = false;
          PlayBackgroundTween(_StartYLocalPos, Ease.InQuad);
        }
      }

      private void PlayBackgroundTween(float targetY, Ease easing) {
        if (_BackgroundTween != null) {
          _BackgroundTween.Kill();
        }
        _BackgroundTween = DOTween.Sequence();
        _BackgroundTween.Append(_BackgroundImageContainer.transform.DOLocalMoveY(
          targetY,
          _GrowTweenDurationSeconds).SetEase(easing));
      }

      public void HideCaratOffscreenRight() {
        PlayCaratTween(_CaratRightOffscreenLocalXPos, _CaratExitTweenDurationSeconds, isWorldPos: false);
      }

      public void HideCaratOffscreenLeft() {
        PlayCaratTween(_CaratLeftOffscreenLocalXPos, _CaratExitTweenDurationSeconds, isWorldPos: false);
      }

      public void MoveCarat(float xWorldPos) {
        PlayCaratTween(xWorldPos, _CaratTweenDurationSeconds, isWorldPos: true);
      }

      private void PlayCaratTween(float targetPos, float duration, bool isWorldPos) {
        if (_CaratTween != null) {
          _CaratTween.Kill();
        }
        if (isWorldPos) {
          _CaratTween = _CaratContainer.transform.DOMoveX(targetPos, 
            _CaratTweenDurationSeconds).SetEase(Ease.OutBack);
        }
        else {
          _CaratTween = _CaratContainer.transform.DOLocalMoveX(targetPos, 
            _CaratTweenDurationSeconds).SetEase(Ease.OutBack);
        }
        _CaratTween.Play();
      }

      public GameObject AddContent(MonoBehaviour contentPrefab, TweenCallback endInTweenCallback) {
        if (_ContentObject != null) {
          Destroy(_ContentObject);
        }
        if (_ContentTween != null) {
          _ContentTween.Kill();
        }
        _ContentObject = UIManager.CreateUIElement(contentPrefab, _ContentContainer.transform);
        _ContentContainer.interactable = false;
        _ContentContainer.alpha = 0;
        _ContentTween = DOTween.Sequence();
        _ContentTween.Append(_ContentContainer.DOFade(1, _ContentFadeTweenDurationSeconds));
        _ContentTween.Join(_ContentContainer.transform.DOLocalMoveX(
          -_ContentTweenXOffset, _ContentFadeTweenDurationSeconds).From().SetEase(Ease.OutQuad));
        _ContentTween.AppendCallback(AddContentFinished);
        if (endInTweenCallback != null) {
          _ContentTween.AppendCallback(endInTweenCallback);
        }

        return _ContentObject;
      }

      private void AddContentFinished() {
        if (_ContentContainer != null) {
          _ContentContainer.interactable = true;
        }
      }

      public void HideContent() {
        if (_ContentObject != null) {
          _ContentContainer.interactable = false;
          _ContentContainer.alpha = 1;
          if (_ContentTween != null) {
            _ContentTween.Kill();
          }
          _ContentTween = DOTween.Sequence();
          _ContentTween.Append(_ContentContainer.DOFade(0, _ContentFadeTweenDurationSeconds));
          _ContentTween.Join(_ContentContainer.transform.DOLocalMoveX(
            _ContentTweenXOffset, _ContentFadeTweenDurationSeconds).SetEase(Ease.OutQuad));
          _ContentTween.AppendCallback(HideContentFinished);
        }
      }

      private void HideContentFinished() {
        if (_ContentObject != null) {
          Destroy(_ContentObject);
          _ContentObject = null;
        }
      }

      public void ShowBackground(bool show) {
        _BackgroundImage.gameObject.SetActive(show);
        _CaratContainer.gameObject.SetActive(show);
      }

      public void PlayBannerAnimation(string textToDisplay, TweenCallback animationEndCallback = null, float customSlowDurationSeconds = 0f) {
        _BannerContainer.gameObject.SetActive(true);
        Vector3 localPos = _BannerContainer.gameObject.transform.localPosition;
        localPos.x = _BannerLeftOffscreenLocalXPos;
        _BannerContainer.gameObject.transform.localPosition = localPos;

        // set text
        _BannerTextLabel.text = textToDisplay;

        float slowDuration = (customSlowDurationSeconds != 0) ? customSlowDurationSeconds : _BannerSlowAnimationDurationSeconds;

        // build sequence
        if (_BannerTween != null) {
          _BannerTween.Kill();
        }
        _BannerTween = DOTween.Sequence();
        float midpoint = (_BannerRightOffscreenLocalXPos + _BannerLeftOffscreenLocalXPos) * 0.5f;
        _BannerTween.Append(_BannerContainer.DOLocalMoveX(midpoint - _BannerSlowDistance, _BannerInOutAnimationDurationSeconds).SetEase(Ease.OutQuad));
        _BannerTween.Append(_BannerContainer.DOLocalMoveX(midpoint, slowDuration));
        _BannerTween.Append(_BannerContainer.DOLocalMoveX(_BannerRightOffscreenLocalXPos, _BannerInOutAnimationDurationSeconds).SetEase(Ease.InQuad));
        _BannerTween.AppendCallback(HandleBannerAnimationEnd);
        if (animationEndCallback != null) {
          _BannerTween.AppendCallback(animationEndCallback);
        }
      }

      private void HandleBannerAnimationEnd() {
        _BannerContainer.gameObject.SetActive(false);
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
