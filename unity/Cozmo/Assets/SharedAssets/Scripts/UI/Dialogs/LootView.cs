using UnityEngine;
using System;
using System.Collections;
using UnityEngine.UI;
using Anki.UI;
using DG.Tweening;

namespace Cozmo {
  namespace UI {
    public class LootView : BaseView {

      private const float kMinScale = 0.8f;
      private const float kMaxScale = 1.2f;
      private const float kMinShake = 5.0f;
      private const float kMaxShake = 15.0f;
      private const float kShakeInterval = 0.01f;
      private const float kChargePerTap = 0.15f;
      private const float kChargeDecay = 0.05f;
      private float _currentCharge = 0.0f;

      [SerializeField]
      private string _LootStartKey;
      [SerializeField]
      private string _LootMidKey;
      [SerializeField]
      private string _LootAlmostKey;
      private const float kLootMidTreshold = 0.3f;
      private const float kLootAlmostThreshold = 0.6f;

      public string LootText {
        get { return _LootText != null ? _LootText.text : null; }
        set {
          if (_LootText != null) {
            _LootText.text = Localization.Get(value);
          }
        }
      }


      [SerializeField]
      private AnkiTextLabel _LootText;

      [SerializeField]
      private Cozmo.UI.CozmoButton _LootButton;

      [SerializeField]
      private Transform _LootBox;

      [SerializeField]
      private Image _LootGlow;

      [SerializeField]
      private CanvasGroup _AlphaController;

      [SerializeField]
      private ProgressBar _ChargeBar;

      private bool _BoxOpened;
      private bool _ShakeStarted;

      private void Awake() {
        Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.UI.WindowOpen);
        _LootGlow.DOFade(0.0f, 0.0f);
        _LootBox.DOScale(kMinScale, 0.0f);
        _BoxOpened = false;
        _ShakeStarted = false;
      }

      private void Update() {
        // Decay charge if the box hasn't been opened yet, update the progressbar.
        if (_BoxOpened == false) {
          if (_currentCharge > 0.0f) {
            if (_ShakeStarted == false) {
              ShakeTheBox();
              _ShakeStarted = true;
            }
            else {
              _currentCharge -= kChargeDecay;
            }
          }
          else {
            _ShakeStarted = false;
            _currentCharge = 0.0f;
          }
          _ChargeBar.SetProgress(_currentCharge);
        }
      }

      private void ShakeTheBox() {
        if (_currentCharge >= 1.0f) {
          // TODO: complete sequence, hide box, play sounds/effects/ 
          _BoxOpened = true;
          _LootBox.gameObject.SetActive(false);
          _ChargeBar.SetProgress(1.0f);
        }
        else if (_currentCharge > 0.0f) {
          // Keep Shaking, Update scale and glow alpha
          // Target Scale determined by current charge and constants.
          float scaleDiff = kMaxScale - kMinScale;
          float currScale = kMinScale + _currentCharge * scaleDiff;
          _LootBox.DOScale(currScale, kShakeInterval);
          // Loot Glow alpha determined by current charge
          _LootGlow.DOFade(_currentCharge, kShakeInterval);

          // Shake Power determined by current charge and constants.
          float shakeDiff = kMaxShake - kMinShake;
          float currShake = kMinShake + _currentCharge * shakeDiff;
          float shakeX = UnityEngine.Random.Range(-currShake, currShake);
          float shakeY = UnityEngine.Random.Range(-currShake, currShake);
          _LootBox.DOMove(new Vector2(shakeX, shakeY), kShakeInterval, false).SetEase(Ease.InOutCubic).SetRelative(true).OnComplete(ShakeTheBox);
          UpdateLootText();
        }
      }

      /// <summary>
      /// Updates the loot text based on the current charge level.
      /// </summary>
      private void UpdateLootText() {
        if (_currentCharge >= kLootAlmostThreshold) {
          LootText = _LootAlmostKey;
        }
        else if (_currentCharge >= kLootMidTreshold) {
          LootText = _LootMidKey;
        }
        else {
          LootText = _LootStartKey;
        }
      }

      protected override void CleanUp() {
      }

      protected override void ConstructOpenAnimation(Sequence openAnimation) {
        openAnimation.Append(transform.DOLocalMoveY(
          50, 0.15f).From().SetEase(Ease.OutQuad).SetRelative());
        if (_AlphaController != null) {
          _AlphaController.alpha = 0;
          openAnimation.Join(_AlphaController.DOFade(1, 0.25f).SetEase(Ease.OutQuad));
        }
      }

      protected override void ConstructCloseAnimation(Sequence closeAnimation) {
        closeAnimation.Append(transform.DOLocalMoveY(
          -50, 0.15f).SetEase(Ease.OutQuad).SetRelative());
        if (_AlphaController != null) {
          closeAnimation.Join(_AlphaController.DOFade(0, 0.25f));
        }
      }
    }
  }
}
