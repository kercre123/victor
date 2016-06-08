using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;
using Anki.UI;
using Anki.Cozmo;
using DG.Tweening;
using Cozmo.MinigameWidgets;

namespace Cozmo {
  namespace UI {
    public class LootView : BaseView {

      #region Constants

      // Initial Box Tween settings
      private const float kBoxIntroDuration = 0.75f;
      private const float kBoxIntroMinScale = 0.15f;

      #region Shake Settings

      private const float kShakeDuration = 1.0f;
      // Rotation Shake Settings
      private const float kShakeRotationMin = 15.0f;
      private const float kShakeRotationMax = 30.0f;
      private const int kShakeRotationVibrato = 45;
      private const float kShakeRotationRandomness = 45.0f;
      // Position Shake Settings
      private const float kShakePositionMin = 15f;
      private const float kShakePositionMax = 45f;
      private const int kShakePositionVibrato = 45;
      private const float kShakePositionRandomness = 30.0f;

      #endregion

      // Rate at which "Recent Tap" Effects decay - includes glow and scaling
      private const float kTapDecay = 0.02f;
      private const float kMinScale = 0.75f;
      private const float kMaxScale = 1.0f;
      private const float kUpdateInterval = 0.35f;
      private float _UpdateTimeStamp = -1;

      // Total Charge per Tap and rate at which Charge decays
      // Charge opens the box at 1.0f and also controls TronBurst and Particle emission rates
      private const float kChargePerTap = 0.15f;
      private const float kChargeDecay = 0.0045f;

      // How long the Reward animation takes to tween the reward doobers to their initial positions
      private const float kDooberExplosionDuration = 0.5f;
      // How long the Rewards remain visible before leaving
      private const float kDooberStayDuration = 1.5f;
      // How long the Reward animation takes to tween the reward doobers to their final positions
      private const float kDooberReturnDuration = 0.75f;
      // The maximum amount of variance in seconds that Doobers are randomly staggered by to create
      // less uniform movements
      private const float kDooberStaggerMax = 0.5f;

      #region Particle Update Settings

      // Tron Line settings
      private const int kTronBurstLow = 3;
      private const int kTronBurstMed = 5;
      private const int kTronBurstHigh = 8;

      // Particle Burst settings for tap burst
      private const int kMinBurst = 4;
      private const int kMaxBurst = 10;
      private const int kFinalBurst = 15;

      #endregion

      private const float kLootMidTreshold = 0.3f;
      private const float kLootAlmostThreshold = 0.6f;

      #endregion

      private float _currentCharge = 0.0f;
      private float _recentTapCharge = 0.0f;
      private int _tronBurst = 1;

      public Dictionary<string, int> LootBoxRewards = new Dictionary<string, int>();

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
      private Transform _BoxSource;
      [SerializeField]
      private Transform _BannerContainer;
      [SerializeField]
      private GameObject _RewardDooberPrefab;
      [SerializeField]
      private GameObject _TronLinePrefab;
      [SerializeField]
      private ParticleSystem _BurstParticles;

      [SerializeField]
      private BannerWidget _BannerWidgetPrefab;
      private BannerWidget _BannerWidgetInstance;

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
      private Sequence _BoxSequence;

      [SerializeField]
      private CanvasGroup _AlphaController;

      private bool _BoxOpened;

      private void Awake() {
        Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.UI.WindowOpen);
        _LootGlow.DOFade(0.0f, 0.0f);
        CreateBoxAnimation();
        _BoxOpened = false;
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(BehaviorGameFlag.NoGame);
        }
        _LootText.gameObject.SetActive(false);

        GameObject newWidget = UIManager.CreateUIElement(_BannerWidgetPrefab.gameObject, _BannerContainer);
        _BannerWidgetInstance = newWidget.GetComponent<BannerWidget>();
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
            StopTweens();

            float currShake = Mathf.Lerp(kShakeRotationMin, kShakeRotationMax, _currentCharge);
            _ShakeRotationTweener = _LootBox.DOShakeRotation(kShakeDuration, new Vector3(0, 0, currShake), kShakeRotationVibrato, kShakeRotationRandomness);
            currShake = Mathf.Lerp(kShakePositionMin, kShakePositionMax, _recentTapCharge);
            _ShakePositionTweener = _LootBox.DOShakePosition(kShakeDuration, currShake, kShakePositionVibrato, kShakePositionRandomness);
          }
        }
      }

      private void Update() {
        // Decay charge if the box hasn't been opened yet, update the progressbar.
        if (_BoxOpened == false) {
          if (_currentCharge > 0.0f) {
            _currentCharge -= kChargeDecay;
            if (Time.time - _UpdateTimeStamp > kUpdateInterval) {
              _UpdateTimeStamp = Time.time;
              float toBurst = Mathf.Lerp(kMinBurst, kMaxBurst, _currentCharge);
              _BurstParticles.Emit((int)toBurst);
              TronLineBurst(_tronBurst);
            }
            if (_currentCharge <= 0.0f) {
              _currentCharge = 0.0f;
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

      private void RewardLoot() {
        _BurstParticles.Emit(kFinalBurst);
        UnleashTheDoobers();
      }

      private Transform SpawnDoober(string rewardID) {
        Transform newDoob = UIManager.CreateUIElement(_RewardDooberPrefab.gameObject, _DooberStart).transform;
        // TODO: Initialize Doober with appropriate values
        if (ItemDataConfig.GetData(rewardID) != null) {
          Sprite rewardIcon = ItemDataConfig.GetData(rewardID).Icon;
          if (rewardIcon != null) {
            newDoob.GetComponent<Image>().overrideSprite = rewardIcon;
          }
        }
        _ActiveDooberTransforms.Add(newDoob);
        return newDoob;
      }

      /// <summary>
      /// Unleashes the doobers. Creates all necessary doobers and tweens them outwards to scattered positions.
      /// </summary>
      /// <param name="doobCount">Doob count.</param>
      private void UnleashTheDoobers() {
        List<Transform> doobTargets = new List<Transform>();
        int doobCount = 0;
        doobTargets.AddRange(_MultRewardsTransforms);
        Sequence dooberSequence = DOTween.Sequence();
        foreach (string itemID in LootBoxRewards.Keys) {
          doobCount += LootBoxRewards[itemID];
          if (doobCount == 1) {
            Transform newDoob = SpawnDoober(itemID);
            dooberSequence.Join(newDoob.DOScale(0.0f, kDooberExplosionDuration).From().SetEase(Ease.OutBack));
          }
          else {
            for (int i = 0; i < LootBoxRewards[itemID]; i++) {
              if (doobTargets.Count <= 0) {
                break;
              }
              Transform newDoob = SpawnDoober(itemID);
              Transform toRemove = doobTargets[UnityEngine.Random.Range(0, doobTargets.Count)];
              Vector3 doobTarget = toRemove.position;
              doobTargets.Remove(toRemove);
              dooberSequence.Join(newDoob.DOScale(0.0f, kDooberExplosionDuration).From().SetEase(Ease.OutBack));
              dooberSequence.Join(newDoob.DOMove(doobTarget, kDooberExplosionDuration).SetEase(Ease.OutBack));
            }
          }
        }

        dooberSequence.InsertCallback(kDooberExplosionDuration + kDooberStayDuration, CloseView);
        dooberSequence.Play();

        ChestRewardManager.Instance.PendingRewards.Clear();
      }

      private void TronLineBurst(int count) {
        for (int i = 0; i < count; i++) {
          UIManager.CreateUIElement(_TronLinePrefab.gameObject, _AlphaController.transform);
        }
      }

      // BEGONE DOOBERS! OUT OF MY SIGHT! Staggering their start times slightly, send all active doobers to the
      // FinalRewardTarget position
      private void SendDoobersAway(Sequence closeAnimation) {
        Sequence dooberSequence = closeAnimation;
        // If there's more than one reward, give each of them a unique transform to tween out to
        for (int i = 0; i < _ActiveDooberTransforms.Count; i++) {
          Transform currDoob = _ActiveDooberTransforms[i];
          float doobStagger = UnityEngine.Random.Range(0, kDooberStaggerMax);
          dooberSequence.Insert(doobStagger, currDoob.DOScale(0.0f, kDooberReturnDuration).SetEase(Ease.InBack));
          dooberSequence.Insert(doobStagger, currDoob.DOMove(_FinalRewardTarget.position, kDooberReturnDuration).SetEase(Ease.InBack));
        }
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
      }

      protected override void ConstructOpenAnimation(Sequence openAnimation) {
        if (_AlphaController != null) {
          _AlphaController.alpha = 0;
          openAnimation.Join(_AlphaController.DOFade(1, 0.25f).SetEase(Ease.OutQuad));
        }

        openAnimation.OnComplete(() =>
          (_BannerWidgetInstance.PlayBannerAnimation(Localization.Get(LocalizationKeys.kLootAnnounce), StartBoxAnimation)));

      }

      public void StartBoxAnimation() {
        _BoxSequence.TogglePause();
      }

      public void CreateBoxAnimation() {
        Sequence boxSequence = DOTween.Sequence();
        boxSequence.Append(_LootBox.DOScale(kBoxIntroMinScale, kBoxIntroDuration).From().SetEase(Ease.InExpo));
        boxSequence.Join(_LootBox.DOLocalMove(_BoxSource.localPosition, kBoxIntroDuration).From());
        boxSequence.OnComplete(HandleBoxFinished);
        boxSequence.Play();
        boxSequence.TogglePause();
        _BoxSequence = boxSequence;
      }

      // Finish open animation, enable LootButton, create initial shake and flourish
      private void HandleBoxFinished() {
        _LootButton.onClick.AddListener(HandleButtonTap);
        _ShakeRotationTweener = _LootBox.DOShakeRotation(kShakeDuration, new Vector3(0, 0, kShakeRotationMax), kShakeRotationVibrato, kShakeRotationRandomness);
        _ShakePositionTweener = _LootBox.DOShakePosition(kShakeDuration, kShakePositionMax, kShakePositionVibrato, kShakePositionRandomness);
        _BurstParticles.Emit(kFinalBurst);
        TronLineBurst(kFinalBurst);
        _LootText.gameObject.SetActive(true);
      }

      protected override void ConstructCloseAnimation(Sequence closeAnimation) {
        if (_AlphaController != null) {
          closeAnimation.Join(_AlphaController.DOFade(0, 0.25f));
        }
        
        SendDoobersAway(closeAnimation);
      }
    }
  }
}
