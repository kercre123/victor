using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;
using Anki.UI;

namespace Cozmo {
  namespace MinigameWidgets {

    public class Banner : MonoBehaviour {

      private const float kAnimXOffset = 0.0f;
      private const float kAnimYOffset = -476.0f;
      private const float kAnimDur = 0.25f;

      [SerializeField]
      private RectTransform _BannerContainer;

      [SerializeField]
      private AnkiTextLegacy _BannerTextLabel;

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

      [SerializeField, Tooltip("PlaySound component to trigger on play (optional)")]
      private Anki.Cozmo.Audio.PlaySound _PlaySound;

      private Sequence _BannerTween = null;

      private void Start() {
        transform.SetAsFirstSibling();
        _BannerContainer.gameObject.SetActive(false);
      }

      private void OnDestroy() {
        if (_BannerTween != null) {
          _BannerTween.Kill();
        }
      }


      public void PlayBannerAnimation(string textToDisplay, TweenCallback animationEndCallback = null, float customSlowDurationSeconds = 0f, bool playSound = true) {
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
        // Play Audio
        _BannerTween.OnStart(() => {
          if (_PlaySound != null && playSound) {
            _PlaySound.Play();
          }
        });
        // Animate banner movement
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
    }
  }
}
