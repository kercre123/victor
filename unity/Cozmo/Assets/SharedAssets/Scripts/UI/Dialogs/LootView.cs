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

      // Rotation and Position Shaking
      private const float kShakeDuration = 1.25f;
      private const float kShakeRotationMin = 20.0f;
      private const float kShakeRotationMax = 45.0f;
      private const int kShakeRotationVibrato = 60;
      private const float kShakeRotationRandomness = 60.0f;
      private const float kShakePositionMin = 15f;
      private const float kShakePositionMax = 25f;
      private const int kShakePositionVibrato = 45;
      private const float kShakePositionRandomness = 30.0f;

      // Rate at which "Recent Tap" Effects decay - includes glow and scaling
      private const float kTapDecay = 0.01f;
      private const float kMinScale = 0.75f;
      private const float kMaxScale = 1.0f;
      private const float kUpdateInterval = 0.25f;

      // Particle Burst settings for tap burst
      private const int kMinBurst = 3;
      private const int kMaxBurst = 6;
      private const int kFinalBurst = 15;

      // Total Charge per Tap and rate at which Charge decays
      private const float kChargePerTap = 0.125f;
      private const float kChargeDecay = 0.0025f;

      // How long the Reward animation takes to tween the reward doobers to their initial positions
      private const float kDooberExplosionDuration = 0.35f;
      // How long the Rewards remain visible before leaving
      private const float kDooberStayDuration = 0.5f;
      // How long the Reward animation takes to tween the reward doobers to their final positions
      private const float kDooberReturnDuration = 0.75f;
      // The maximum amount of variance in seconds that Doobers are randomly staggered by to create
      // less uniform movements
      private const float kDooberStaggerMax = 0.75f;

      // Tron Line settings
      private const int kTronBurstLow = 3;
      private const int kTronBurstMed = 6;
      private const int kTronBurstHigh = 10;

      private float _currentCharge = 0.0f;
      private float _recentTapCharge = 0.0f;
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
      private GameObject _TronLinePrefab;
      [SerializeField]
      private ParticleSystem _BurstParticles;

      private const float kLootMidTreshold = 0.20f;
      private const float kLootAlmostThreshold = 0.55f;

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

      private Tweener _ShakeRotationTweener;
      private Tweener _ShakePositionTweener;
      //private Tweener _ScaleTweener;

      [SerializeField]
      private CanvasGroup _AlphaController;

      private bool _BoxOpened;
      private bool _ChargeStarted;

      private void Awake() {
        Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.UI.WindowOpen);
        _LootGlow.DOFade(0.0f, 0.0f);
        _LootBox.DOScale(kMinScale, 0.0f);
        _BoxOpened = false;
        _ChargeStarted = false;
        _LootButton.onClick.AddListener(HandleButtonTap);
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(BehaviorGameFlag.NoGame);
        }

      }

      // Handle each tap
      private void HandleButtonTap() {
        if (_BoxOpened == false) {

          if (_currentCharge >= 1.0f) {
            _BoxOpened = true;

            StopTweens();
            _LootBox.gameObject.SetActive(false);
            RewardLoot();
          }
          else {
            _currentCharge += kChargePerTap;
            _recentTapCharge = 1.0f;

            float toBurst = Mathf.Lerp(kMinBurst, kMaxBurst, _currentCharge);
            _BurstParticles.Emit((int)toBurst);
            TronLineBurst(_tronBurst);

            float currShake = Mathf.Lerp(kShakeRotationMin, kShakeRotationMax, _currentCharge);
            if (_ShakeRotationTweener != null) {
              _ShakeRotationTweener.Complete();
            }
            _ShakeRotationTweener = _LootBox.DOShakeRotation(kShakeDuration, new Vector3(0, 0, currShake), kShakeRotationVibrato, kShakeRotationRandomness);
            currShake = Mathf.Lerp(kShakePositionMin, kShakePositionMax, _recentTapCharge);
            if (_ShakePositionTweener != null) {
              _ShakePositionTweener.Complete();
            }
            _ShakePositionTweener = _LootBox.DOShakePosition(kShakeDuration, currShake, kShakePositionVibrato, kShakePositionRandomness);
          }

        }
      }

      private void Update() {
        // Decay charge if the box hasn't been opened yet, update the progressbar.
        if (_BoxOpened == false) {
          if (_currentCharge > 0.0f) {
            if (_ChargeStarted == false) {
              //BoxChargeUpdate();
              _ChargeStarted = true;
            }
            else {
              _currentCharge -= kChargeDecay;
              if (_currentCharge <= 0.0f) {
                _ChargeStarted = false;
                _currentCharge = 0.0f;
              }
            }
          }
          if (_recentTapCharge > 0.0f) {
            _recentTapCharge -= kTapDecay;
            if (_recentTapCharge <= 0.0f) {
              _recentTapCharge = 0.0f;
            }
            _LootGlow.color = new Color(_LootGlow.color.r, _LootGlow.color.g, _LootGlow.color.b, _recentTapCharge);
            float newScale = Mathf.Lerp(kMinScale, kMaxScale, _recentTapCharge);
            _LootBox.localScale = new Vector3(newScale, newScale, 1.0f);
            _BoxGlow.color = _LootGlow.color;
          }
          UpdateLootText();
        }
      }

      /*
      private void BoxChargeUpdate() {
        if ((_currentCharge > 0.0f) && (_BoxOpened == false)) {

          // Tron Line Effect

          // Keep Shaking, Update scale and glow alpha
          // Target Scale determined by current charge and constants.
          float currScale = Mathf.Lerp(kMinScale, kMaxScale, _recentTapCharge);
          if (_ScaleTweener != null) {
            _ScaleTweener.Complete();
          }
          // Scale Tween
          _ScaleTweener = _LootBox.DOScale(currScale, kUpdateInterval).OnComplete(BoxChargeUpdate);

        }
      }
      */

      /// <summary>
      /// Updates the loot text based on the current charge level.
      /// </summary>
      private void UpdateLootText() {
        if (_BoxOpened) {
          LootText = string.Empty;
        }
        else if (_currentCharge >= kLootAlmostThreshold) {
          LootText = _LootAlmostKey;
          _tronBurst = kTronBurstHigh;
        }
        else if (_currentCharge >= kLootMidTreshold) {
          LootText = _LootMidKey;
          _tronBurst = kTronBurstMed;
        }
        else {
          LootText = _LootStartKey;
          _tronBurst = kTronBurstLow;
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
          UIManager.CreateUIElement(_TronLinePrefab.gameObject, _DooberStart);
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
        StopTweens();
      }

      private void StopTweens() {
        if (_ShakePositionTweener != null) {
          _ShakePositionTweener.Complete();
        }
        if (_ShakeRotationTweener != null) {
          _ShakeRotationTweener.Complete();
        }
        /*
        if (_ScaleTweener != null) {
          _ScaleTweener.Complete();
        }*/
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
