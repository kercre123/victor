using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {

    public class ShelfWidget : MinigameWidget {

      public System.Action GameDoneButtonPressed;

      private const float kAnimXOffset = 0.0f;
      private const float kAnimYOffset = -476.0f;
      private const float kAnimDur = 0.25f;

      [SerializeField]
      private RectTransform _BackgroundContainer;

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
      private Cozmo.UI.CozmoButton _GameDoneButton;

      [SerializeField]
      private Anki.UI.AnkiTextLabel _ShelfText;

      private void Awake() {
        Color transparent = Color.white;
        transparent.a = 0f;
        _ShelfText.gameObject.SetActive(false);
        _ShelfText.text = "";

        _GameDoneButton.Initialize(() => {
          if (GameDoneButtonPressed != null) {
            GameDoneButtonPressed();
          }
        }, "game_done_button", "shelf_widget");
        ShowGameDoneButton(false);
      }

      private void Start() {
        transform.SetAsFirstSibling();
      }

      private void OnDestroy() {
        ResetTween(_ContentTween);
        ResetTween(_BackgroundTween);
        ResetTween(_CaratTween);
      }

      public void SetWidgetText(string widgetTextKey) {
        _ShelfText.gameObject.SetActive(widgetTextKey != string.Empty);
        _ShelfText.text = Localization.Get(widgetTextKey);
      }

      public void ShowGameDoneButton(bool show) {
        _GameDoneButton.gameObject.SetActive(show);
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
        _BackgroundTween.Append(_BackgroundContainer.transform.DOLocalMoveY(
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
        _ContentTween.Append(_ContentContainer.DOFade(1, _ContentFadeTweenDurationSeconds).SetEase(UIDefaultTransitionSettings.Instance.FadeInEasing));
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
          _ContentTween.Append(_ContentContainer.DOFade(0, _ContentFadeTweenDurationSeconds).SetEase(UIDefaultTransitionSettings.Instance.FadeOutEasing));
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
        _CaratContainer.gameObject.SetActive(show);
      }

      private void PlayFadeTween(ref Tweener tween, Image target, float targetAlpha, float duration, Ease easing) {
        ResetTween(tween);
        tween = target.DOFade(targetAlpha, duration).SetEase(easing);
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
