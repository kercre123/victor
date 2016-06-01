using UnityEngine;
using System;
using System.Collections;
using UnityEngine.UI;
using Anki.UI;
using Anki.Cozmo;
using DG.Tweening;

namespace Cozmo {
  namespace UI {
    public class LootView : BaseView {

      private const float kMinScale = 1.0f;
      private const float kMaxScale = 1.25f;
      private const float kMaxShake = 3.5f;
      private const float kShakeInterval = 0.025f;
      private const float kShakeDecay = 0.0075f;

      private const float kChargePerTap = 0.15f;
      private const float kChargeDecay = 0.0025f;

      private float _currentCharge = 0.0f;
      private float _currentShake = 0.0f;

      [SerializeField]
      private string _LootStartKey;
      [SerializeField]
      private string _LootMidKey;
      [SerializeField]
      private string _LootAlmostKey;

      private const float kLootMidTreshold = 0.2f;
      private const float kLootAlmostThreshold = 0.7f;

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
      private Button _LootButton;

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
        _LootButton.onClick.AddListener(HandleButtonTap);
        RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(BehaviorGameFlag.NoGame);
      }

      private void HandleButtonTap() {
        _currentCharge += kChargePerTap;
        _currentShake += kChargePerTap;
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
              if (_currentCharge <= 0.0f) {
                _ShakeStarted = false;
                _currentCharge = 0.0f;
              }
            }
          }
          if (_currentShake > 0.0f) {
            _currentShake -= kShakeDecay;
            if (_currentShake <= 0.0f) {
              _currentShake = 0.0f;
            }
          }
          _ChargeBar.SetProgress(_currentCharge);
        }
      }

      private void ShakeTheBox() {
        if ((_currentCharge > 0.0f) && (_BoxOpened == false)) {
          // Keep Shaking, Update scale and glow alpha
          // Target Scale determined by current charge and constants.
          float scaleDiff = kMaxScale - kMinScale;
          float currScale = kMinScale + _currentCharge * scaleDiff;
          // kShakeInterval is doubled to accomodate for the DoMove loops to allow pingpong effect
          _LootBox.DOScale(currScale, kShakeInterval * 2);
          // Loot Glow alpha determined by current charge
          _LootGlow.DOFade(_currentCharge, kShakeInterval * 2);

          // Shake Power determined by current charge and constants.
          float currShake = _currentShake * kMaxShake;
          float shakeX = UnityEngine.Random.Range(-currShake, currShake);
          float shakeY = UnityEngine.Random.Range(-currShake, currShake);
          //2 loops in order for _LootBox to return to where it begins
          _LootBox.DOMove(new Vector2(shakeX, shakeY), kShakeInterval, false).SetEase(Ease.InOutCubic).SetLoops(2, LoopType.Yoyo).SetRelative(true).OnComplete(ShakeTheBox);
          UpdateLootText();
          if (_currentCharge >= 1.0f) {
            // TODO: complete sequence, hide box, play sounds/effects, start reward animation.
            _BoxOpened = true;
            _LootBox.gameObject.SetActive(false);
            _ChargeBar.SetProgress(1.0f);
            RewardLoot();
          }
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

      private void RewardLoot() {
        // TODO: Start Reward Animation Sequence and create LootDoobers
        CloseView();
      }

      protected override void CleanUp() {
        _LootButton.onClick.RemoveAllListeners();
        RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(BehaviorGameFlag.All);
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
