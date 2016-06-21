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

      public enum CircuitryType {
        RightAlignedForText,
        RightAlignedNoText,
        CenterAligned,
        None
      }

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
      private Banner _BannerWidgetPrefab;
      private Banner _BannerWidgetInstance;

      [SerializeField]
      private Anki.UI.AnkiTextLabel _ShelfText;

      [SerializeField]
      private Image _RightAlignedCircuitryForText;

      [SerializeField]
      private Image _RightAlignedCircuitryForNoText;

      [SerializeField]
      private Image _CenterAlignedCircuitry;

      [SerializeField]
      private float _CircuitryFadeDuration_sec = 0.5f;

      private Tweener _CircuitryTweenIn;
      private Tweener _CircuitryTweenOut;

      private CircuitryType _CurrentCircuitryType;

      private void Awake() {
        Color transparent = Color.white;
        transparent.a = 0f;
        _RightAlignedCircuitryForText.color = Color.white;
        _RightAlignedCircuitryForNoText.color = transparent;
        _CenterAlignedCircuitry.color = transparent;
        _CurrentCircuitryType = CircuitryType.RightAlignedForText;
        _ShelfText.text = "";
      }

      private void Start() {
        transform.SetAsFirstSibling();

        GameObject newWidget = UIManager.CreateUIElement(_BannerWidgetPrefab.gameObject, _BannerContainer);
        _BannerWidgetInstance = newWidget.GetComponent<Banner>();
      }

      private void OnDestroy() {
        ResetTween(_ContentTween);
        ResetTween(_BackgroundTween);
        ResetTween(_CaratTween);
        ResetTween(_CircuitryTweenIn);
        ResetTween(_CircuitryTweenOut);
      }

      public void SetWidgetText(string widgetTextKey) {
        _ShelfText.text = Localization.Get(widgetTextKey);
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
        ResetTween(_BackgroundTween);
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
        ResetTween(_CaratTween);
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
        ResetTween(_ContentTween);
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
          ResetTween(_ContentTween);
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
        _BannerWidgetInstance.PlayBannerAnimation(textToDisplay, animationEndCallback, customSlowDurationSeconds);
      }

      public void SetCircuitry(CircuitryType circuitryType) {
        if (circuitryType != _CurrentCircuitryType) {
          Image activeCircuitry = GetCircuitryImageByType(_CurrentCircuitryType);
          if (activeCircuitry != null) {
            FadeOutCircuitry(activeCircuitry);
          }

          Image newCircuitry = GetCircuitryImageByType(circuitryType);
          if (newCircuitry != null) {
            FadeInCircuitry(newCircuitry);
          }

          _CurrentCircuitryType = circuitryType;
        }
      }

      public Image GetCircuitryImageByType(CircuitryType circuitryType) {
        Image circuitryImage = null;
        switch (circuitryType) {
        case CircuitryType.RightAlignedForText:
          circuitryImage = _RightAlignedCircuitryForText;
          break;
        case CircuitryType.RightAlignedNoText:
          circuitryImage = _RightAlignedCircuitryForNoText;
          break;
        case CircuitryType.CenterAligned:
          circuitryImage = _CenterAlignedCircuitry;
          break;
        default:
          break;
        }
        return circuitryImage;
      }

      private void FadeInCircuitry(Image circuitry) {
        PlayFadeTween(ref _CircuitryTweenIn, circuitry, 1f);
      }

      private void FadeOutCircuitry(Image circuitry) {
        PlayFadeTween(ref _CircuitryTweenOut, circuitry, 0f);
      }

      private void PlayFadeTween(ref Tweener tween, Image target, float targetAlpha) {
        ResetTween(tween);
        tween = target.DOFade(targetAlpha, _CircuitryFadeDuration_sec).SetEase(Ease.OutQuad);
      }

      private void ResetTween(Tween tween) {
        if (tween != null) {
          tween.Kill();
        }
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
