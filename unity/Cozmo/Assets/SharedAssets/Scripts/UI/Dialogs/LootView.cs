using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;
using Anki.UI;
using Anki.Cozmo;
using DG.Tweening;

namespace Cozmo {
  namespace UI {
    public class LootView : BaseView {

      private const float kMinScale = 0.75f;
      private const float kMaxScale = 1.0f;

      private const float kMaxShake = 1f;
      private const float kShakeInterval = 0.10f;
      private const float kRotateShakeDuration = 1.25f;
      private const float kShakeDecay = 0.02f;
      private const float kGlowDecay = 0.01f;
      private const float kShakePerTap = 0.4f;
      private const float kShakeRotation = 75.0f;
      private const int kShakeRotationVibrato = 30;
      private const float kShakeRotationRandomness = 25.0f;

      private const int kMinBurst = 3;
      private const int kMaxBurst = 10;
      private const int kFinalBurst = 25;

      private const float kChargePerTap = 0.20f;
      private const float kChargeDecay = 0.005f;
      // How long the Reward animation takes to tween the reward doobers to their initial positions
      private const float kDooberExplosionDuration = 0.25f;
      // How long the Rewards remain visible before leaving
      private const float kDooberStayDuration = 0.5f;
      // How long the Reward animation takes to tween the reward doobers to their final positions
      private const float kDooberReturnDuration = 0.75f;
      // The maximum amount of variance in seconds that Doobers are randomly staggered by to create
      // less uniform movements
      private const float kDooberStaggerMax = 0.75f;

      private float _currentCharge = 0.0f;
      private float _currentShake = 0.0f;
      private float _currentGlow = 0.0f;
      private int _tronBurst = 1;

      [SerializeField]
      private string _LootStartKey;
      [SerializeField]
      private string _LootMidKey;
      [SerializeField]
      private string _LootAlmostKey;
      [SerializeField]
      private List<Transform> _MultRewardsTransforms;
      [SerializeField]
      private List<Transform> _ActiveDooberTransforms;
      [SerializeField]
      private Transform _DooberStart;
      [SerializeField]
      private Transform _FinalRewardTarget;
      [SerializeField]
      private GameObject _RewardDooberPrefab;
      [SerializeField]
      private GameObject _TronDooberPrefab;
      [SerializeField]
      private ParticleSystem _BurstParticles;

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
      private Image _BoxGlow;

      private Tweener _ShakeTweener;

      [SerializeField]
      private CanvasGroup _AlphaController;

      private bool _BoxOpened;
      private bool _ShakeStarted;

      private void Awake() {
        Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.UI.WindowOpen);
        _LootGlow.DOFade(0.0f, 0.0f);
        _LootBox.DOScale(kMinScale, 0.0f);
        _BoxOpened = false;
        _ShakeStarted = false;
        _LootButton.onClick.AddListener(HandleButtonTap);
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(BehaviorGameFlag.NoGame);
        }

      }

      // TODO: Particle Burst
      private void HandleButtonTap() {
        if (_BoxOpened == false) {
          _currentCharge += kChargePerTap;
          _currentShake += kShakePerTap;
          _currentGlow = 1.0f;
          if (_currentShake > 1.0f) {
            _currentShake = 1.0f;
          }

          float toBurst = Mathf.Lerp(kMinBurst, kMaxBurst, _currentCharge);
          _BurstParticles.Emit((int)toBurst);

          float currShake = Mathf.Lerp(0, kShakeRotation, _currentShake);
          if (_ShakeTweener != null) {
            _ShakeTweener.Complete();
          }
          _ShakeTweener = _LootBox.DOShakeRotation(kRotateShakeDuration, new Vector3(0, 0, currShake), kShakeRotationVibrato, kShakeRotationRandomness);
        }
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
          if (_currentGlow > 0.0f) {
            _currentGlow -= kGlowDecay;
            if (_currentGlow <= 0.0f) {
              _currentGlow = 0.0f;
            }
            _LootGlow.color = new Color(_LootGlow.color.r, _LootGlow.color.g, _LootGlow.color.b, _currentGlow);
            _BoxGlow.color = _LootGlow.color;
          }
          UpdateLootText();
          if (_currentCharge >= 1.0f) {
            // TODO: play sounds/effects
            _BoxOpened = true;
            _LootBox.gameObject.SetActive(false);
            RewardLoot();
          }
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

          // Tron Doober Effect
          TronLineBurst(_tronBurst);
          // Shake Power determined by current charge and constants.
          float currShake = Mathf.Lerp(0, kMaxShake, _currentShake);
          float shakeX = UnityEngine.Random.Range(-currShake, currShake);
          float shakeY = UnityEngine.Random.Range(-currShake, currShake);
          //2 loops in order for _LootBox to return to where it begins
          _LootBox.DOMove(new Vector2(shakeX, shakeY), kShakeInterval, false).SetEase(Ease.InOutCubic).SetLoops(2, LoopType.Yoyo).SetRelative(true).OnComplete(ShakeTheBox);

        }
      }

      /// <summary>
      /// Updates the loot text based on the current charge level.
      /// </summary>
      private void UpdateLootText() {
        if (_BoxOpened) {
          LootText = string.Empty;
        }
        else if (_currentCharge >= kLootAlmostThreshold) {
          LootText = _LootAlmostKey;
          _tronBurst = 8;
        }
        else if (_currentCharge >= kLootMidTreshold) {
          LootText = _LootMidKey;
          _tronBurst = 4;
        }
        else {
          LootText = _LootStartKey;
          _tronBurst = 2;
        }
      }

      private void RewardLoot(/*TODO : pass in full information about the rewards received from the current chest*/) {
        // TODO: Start Reward Animation Sequence and create LootDoobers
        // TODO: Currently is not using Reward information to determine the doobers looted.
        // purely hardcoded bullshit for testing.-R.A.
        _BurstParticles.Emit(kFinalBurst);
        UnleashTheDoobers(UnityEngine.Random.Range(1, 6));
      }

      private Transform SpawnDoober(/*TODO : pass in the name of the rewardID for this Doobster*/) {
        // TODO : This seems to be offset in a weird way, figure out why.
        Transform newDoob = UIManager.CreateUIElement(_RewardDooberPrefab.gameObject, _DooberStart).transform;
        // TODO: Initialize Doober with appropriate values
        _ActiveDooberTransforms.Add(newDoob);
        return newDoob;
      }

      /// <summary>
      /// Unleashes the doobers. Creates all necessary doobers and tweens them outwards to scattered positions.
      /// </summary>
      /// <param name="doobCount">Doob count.</param>
      private void UnleashTheDoobers(int doobCount /*TODO : pass in full information about the rewards received from the current chest*/) {
        List<Transform> doobTargets = new List<Transform>();
        doobTargets.AddRange(_MultRewardsTransforms);
        // TODO : Create targets dynamically limited by doobCount, not other way around
        if (doobCount > doobTargets.Count) {
          doobCount = doobTargets.Count;
        }
        Sequence dooberSequence = DOTween.Sequence();
        // If there's only one reward, put it directly in the middle where the box was
        if (doobCount == 1) {
          Transform newDoob = SpawnDoober();
          dooberSequence.Join(newDoob.DOScale(0.0f, kDooberExplosionDuration).From().SetEase(Ease.OutBack));
        }
        else {
          // If there's more than one reward, give each of them a unique transform to tween out to
          for (int i = 0; i < doobCount; i++) {
            Transform newDoob = SpawnDoober();
            Transform toRemove = doobTargets[UnityEngine.Random.Range(0, doobTargets.Count)];
            Vector3 doobTarget = toRemove.position;
            doobTargets.Remove(toRemove);
            dooberSequence.Join(newDoob.DOScale(0.0f, kDooberExplosionDuration).From().SetEase(Ease.OutBack));
            dooberSequence.Join(newDoob.DOMove(doobTarget, kDooberExplosionDuration).SetEase(Ease.OutBack));
          }
        }

        dooberSequence.InsertCallback(kDooberExplosionDuration + kDooberStayDuration, SendDoobersAway);
        dooberSequence.Play();
      }

      private void TronLineBurst(int count) {
        for (int i = 0; i < count; i++) {
          UIManager.CreateUIElement(_TronDooberPrefab.gameObject, _DooberStart);
        }
      }

      // BEGONE DOOBERS! OUT OF MY SIGHT! Staggering their start times slightly, send all active doobers to the
      // FinalRewardTarget position
      private void SendDoobersAway() {
        Sequence dooberSequence = DOTween.Sequence();
        // If there's more than one reward, give each of them a unique transform to tween out to
        for (int i = 0; i < _ActiveDooberTransforms.Count; i++) {
          Transform currDoob = _ActiveDooberTransforms[i];
          float doobStagger = UnityEngine.Random.Range(0, kDooberStaggerMax);
          dooberSequence.Insert(doobStagger, currDoob.DOScale(0.0f, kDooberReturnDuration).SetEase(Ease.InBack));
          dooberSequence.Insert(doobStagger, currDoob.DOMove(_FinalRewardTarget.position, kDooberReturnDuration).SetEase(Ease.InBack));
        }
        dooberSequence.OnComplete(CloseView);
        dooberSequence.Play();
      }

      protected override void CleanUp() {
        _LootButton.onClick.RemoveAllListeners();
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(BehaviorGameFlag.All);
        }
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
