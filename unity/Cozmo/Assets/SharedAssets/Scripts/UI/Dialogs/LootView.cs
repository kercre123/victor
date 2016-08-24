using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine.UI;
using Anki.UI;
using Anki.Cozmo;
using DG.Tweening;
using Cozmo.MinigameWidgets;
using Cozmo.Util;

namespace Cozmo {
  namespace UI {
    public class LootView : BaseView {

      #region Constants

      // Initial Box Tween settings
      [SerializeField]
      private float _BoxIntroTweenDuration = 0.75f;
      [SerializeField]
      private float _BoxIntroSettleDuration = 0.10f;
      [SerializeField]
      private float _BoxIntroStartScale = 0.15f;

      #region Shake Settings

      [SerializeField]
      private float _ShakeDuration = 1.0f;
      // Rotation Shake Settings
      [SerializeField]
      private float _ShakeRotationMinAngle = 15.0f;
      [SerializeField]
      private float _ShakeRotationMaxAngle = 30.0f;
      [SerializeField]
      private int _ShakeRotationVibrato = 45;
      [SerializeField]
      private float _ShakeRotationRandomness = 45.0f;
      // Position Shake Settings
      [SerializeField]
      private float _ShakePositionMin = 15f;
      [SerializeField]
      private float _ShakePositionMax = 45f;
      [SerializeField]
      private int _ShakePositionVibrato = 45;
      [SerializeField]
      private float _ShakePositionRandomness = 30.0f;

      #endregion

      #region Tap Effect Settings

      // Rate at which "Recent Tap" Effects decay - includes glow and scaling
      [SerializeField]
      private float _TapDecayRate = 0.02f;
      [SerializeField, Tooltip("Resting scale of the box")]
      private float _MinBoxScale = 0.75f;
      [SerializeField, Tooltip("Recently tapped scale of the box")]
      private float _MaxBoxScale = 1.0f;
      [SerializeField]
      private float _ChargeVFXUpdateInterval = 0.35f;

      // Total Charge per Tap and rate at which Charge decays
      // Charge opens the box at 1.0f and also controls TronBurst and Particle emission rates
      [SerializeField]
      private float _ChargePerTap = 0.2f;
      [SerializeField]
      private float _ChargeDecayRate = 0.003f;

      // How long the Reward animation takes to tween the reward doobers to their initial positions
      [SerializeField]
      private float _RewardExplosionDuration = 0.5f;
      // How long the Rewards remain visible before leaving
      [SerializeField]
      private float _RewardExplosionStayDuration = 1.5f;
      // How long the Reward animation takes to tween the reward doobers to their final positions
      [SerializeField]
      private float _RewardExplosionFinalDuration = 1.25f;
      // The maximum amount of variance in seconds that Doobers are randomly staggered by to create
      // less uniform movements
      [SerializeField]
      private float _RewardExplosionFinalVariance = 1.0f;

      #region Particle Update Settings

      // Tron Line settings
      [SerializeField]
      private int _TronBurstLow = 3;
      [SerializeField]
      private int _TronBurstMid = 5;
      [SerializeField]
      private int _TronBurstAlmost = 8;

      // Particle Burst settings for tap burst
      [SerializeField]
      private int _MinParticleBurst = 4;
      [SerializeField]
      private int _MaxParticleBurst = 10;
      [SerializeField]
      private int _OpenChestBurst = 15;

      [SerializeField, Tooltip("Charge % where TronBurst and Text go to Midtier")]
      private float _LootMidThreshold = 0.3f;
      [SerializeField, Tooltip("Charge % where TronBurst and Text go to almost tier")]
      private float _LootAlmostThreshold = 0.6f;

      #endregion

      #endregion

      #endregion

      #region Audio

      [SerializeField]
      private Anki.Cozmo.Audio.AudioEventParameter _TapSoundEvent = Anki.Cozmo.Audio.AudioEventParameter.DefaultClick;
      [SerializeField]
      private Anki.Cozmo.Audio.AudioEventParameter _EmotionChipWindowOpenSoundEvent = Anki.Cozmo.Audio.AudioEventParameter.DefaultClick;
      [SerializeField]
      private Anki.Cozmo.Audio.AudioEventParameter _LootReleasedSoundEvent = Anki.Cozmo.Audio.AudioEventParameter.DefaultClick;

      #endregion

      private float _currentCharge = 0.0f;
      private float _recentTapCharge = 0.0f;
      private int _tronBurst = 1;
      private float _UpdateTimeStamp = -1;

      SimpleObjectPool<TronLight> _TronPool;

      public Dictionary<string, int> LootBoxRewards = new Dictionary<string, int>();

      [SerializeField]
      private string _LootStartKey;
      [SerializeField]
      private string _LootMidKey;
      [SerializeField]
      private string _LootAlmostKey;
      [SerializeField]
      private List<Transform> _MultRewardsTransforms;
      private List<Transform> _ActiveBitsTransforms;
      private List<Transform> _ActiveSparkTransforms;
      [SerializeField]
      private Transform _DooberStart;
      [SerializeField]
      private Transform _FinalBitsTarget;
      [SerializeField]
      private Transform _FinalSparkTarget;
      [SerializeField]
      private Transform _BoxSource;
      [SerializeField]
      private Transform _BannerContainer;
      [SerializeField]
      private GameObject _RewardDooberPrefab;
      [SerializeField]
      private GameObject _TronLightPrefab;
      [SerializeField]
      private ParticleSystem _BurstParticles;

      [SerializeField]
      private Banner _BannerPrefab;
      private Banner _BannerInstance;

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

      #region Onboarding
      [SerializeField]
      private AnkiTextLabel _LootTextInstructions1;
      [SerializeField]
      private AnkiTextLabel _LootTextInstructions2;
      // Very special pause and sorting
      [SerializeField]
      private List<Transform> _OnboardingRewardTransforms1;
      [SerializeField]
      private List<Transform> _OnboardingRewardTransforms2;
      [SerializeField]
      private Transform _OnboardingDooberStart;
      [SerializeField]
      private CozmoButton _ContinueButtonInstance;
      #endregion

      private bool _BoxOpened;

      private void Awake() {
        Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_EmotionChipWindowOpenSoundEvent);
        _TronPool = new SimpleObjectPool<TronLight>(CreateTronLight, ResetTronLight, 0);
        _ActiveBitsTransforms = new List<Transform>();
        _ActiveSparkTransforms = new List<Transform>();
        ContextManager.Instance.AppFlash(playChime: true);
        ContextManager.Instance.CozmoHoldFreeplayStart();
        Color transparentColor = new Color(_LootGlow.color.r, _LootGlow.color.g, _LootGlow.color.b, 0);
        _LootGlow.color = transparentColor;

        StartCoroutine(InitializeBox());
        _BoxOpened = false;
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(BehaviorGameFlag.NoGame);
        }
        _LootText.gameObject.SetActive(false);

        GameObject banner = UIManager.CreateUIElement(_BannerPrefab.gameObject, _BannerContainer);
        _BannerInstance = banner.GetComponent<Banner>();

        if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Loot)) {
          OnboardingManager.Instance.StartPhase(OnboardingManager.OnboardingPhases.Loot);
        }
      }

      private IEnumerator InitializeBox() {
        yield return new WaitForFixedUpdate();
        _LootBox.gameObject.SetActive(true);
        CreateBoxAnimation();
      }

      #region tronlight pool logic

      private TronLight CreateTronLight() {
        TronLight light = GameObject.Instantiate<GameObject>(_TronLightPrefab).GetComponent<TronLight>();
        light.transform.SetParent(_AlphaController.transform, false);
        light.OnLifeSpanEnd += HandleTronLightEnd;
        light.Initialize();
        return light;
      }

      private void HandleTronLightEnd(TronLight light) {
        light.OnLifeSpanEnd -= HandleTronLightEnd;
        light.OnPool();
        _TronPool.ReturnToPool(light);
      }

      private void ResetTronLight(TronLight entry, bool spawned) {
        entry.gameObject.SetActive(spawned);
        if (spawned) {
          entry.OnLifeSpanEnd += HandleTronLightEnd;
          entry.Initialize();
        }
      }

      #endregion

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
            _currentCharge += _ChargePerTap;
            _recentTapCharge = 1.0f;
            StopTweens();

            float currShake = Mathf.Lerp(_ShakeRotationMinAngle, _ShakeRotationMaxAngle, _currentCharge);
            _ShakeRotationTweener = _LootBox.DOShakeRotation(_ShakeDuration, new Vector3(0, 0, currShake), _ShakeRotationVibrato, _ShakeRotationRandomness);
            currShake = Mathf.Lerp(_ShakePositionMin, _ShakePositionMax, _currentCharge);
            _ShakePositionTweener = _LootBox.DOShakePosition(_ShakeDuration, currShake, _ShakePositionVibrato, _ShakePositionRandomness);
            Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_TapSoundEvent);
          }
        }
      }

      private void Update() {
        // Decay charge if the box hasn't been opened yet, update the progressbar.
        if (_BoxOpened == false) {
          if (_currentCharge > 0.0f) {
            _currentCharge -= _ChargeDecayRate;
            if (Time.time - _UpdateTimeStamp > _ChargeVFXUpdateInterval) {
              _UpdateTimeStamp = Time.time;
              float toBurst = Mathf.Lerp(_MinParticleBurst, _MaxParticleBurst, _currentCharge);
              _BurstParticles.Emit((int)toBurst);
              TronLineBurst(_tronBurst);
            }
            if (_currentCharge <= 0.0f) {
              _currentCharge = 0.0f;
            }
          }
          if (_recentTapCharge > 0.0f) {
            _recentTapCharge -= _TapDecayRate;
            if (_recentTapCharge <= 0.0f) {
              _recentTapCharge = 0.0f;
            }
            _LootGlow.color = new Color(_LootGlow.color.r, _LootGlow.color.g, _LootGlow.color.b, _recentTapCharge);
            float newScale = Mathf.Lerp(_MinBoxScale, _MaxBoxScale, _recentTapCharge);
            _LootBox.localScale = new Vector3(newScale, newScale, 1.0f);
            _BoxGlow.color = _LootGlow.color;
          }
        }
        UpdateLootText();
      }

      /// <summary>
      /// Updates the loot text based on the current charge level.
      /// </summary>
      private void UpdateLootText() {
        if (_BoxOpened) {
          _LootText.gameObject.SetActive(false);
          _LootTextInstructions1.gameObject.SetActive(false);
          _LootTextInstructions2.gameObject.SetActive(false);
        }
        else if (_currentCharge >= _LootAlmostThreshold) {
          LootText = _LootAlmostKey;
          _tronBurst = _TronBurstAlmost;
        }
        else if (_currentCharge >= _LootMidThreshold) {
          LootText = _LootMidKey;
          _tronBurst = _TronBurstMid;
        }
        else {
          LootText = _LootStartKey;
          _tronBurst = _TronBurstLow;
        }
      }

      private void RewardLoot() {
        _BurstParticles.Emit(_OpenChestBurst);
        Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_LootReleasedSoundEvent);
        if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Loot)) {
          UnleashTheDoobersForOnboarding();
        }
        else {
          UnleashTheDoobers();
        }
      }

      private Transform SpawnDoober(string rewardID) {
        Transform newDoob = UIManager.CreateUIElement(_RewardDooberPrefab.gameObject, _DooberStart).transform;
        Sprite rewardIcon = null;
        // TODO: Initialize Doober with appropriate values
        ItemData iData = ItemDataConfig.GetData(rewardID);
        if (iData != null) {
          rewardIcon = iData.Icon;
        }

        if (rewardIcon != null) {
          newDoob.GetComponent<Image>().overrideSprite = rewardIcon;
        }
        if (rewardID == RewardedActionManager.Instance.SparkID) {
          _ActiveSparkTransforms.Add(newDoob);
        }
        else {
          _ActiveBitsTransforms.Add(newDoob);
        }
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
            dooberSequence.Join(newDoob.DOScale(0.0f, _RewardExplosionDuration).From().SetEase(Ease.OutBack));
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
              dooberSequence.Join(newDoob.DOScale(0.0f, _RewardExplosionDuration).From().SetEase(Ease.OutBack));
              dooberSequence.Join(newDoob.DOMove(doobTarget, _RewardExplosionDuration).SetEase(Ease.OutBack));
            }
          }
        }

        dooberSequence.InsertCallback(_RewardExplosionDuration + _RewardExplosionStayDuration, CloseView);
        dooberSequence.Play();

        ChestRewardManager.Instance.PendingChestRewards.Clear();
        ContextManager.Instance.CozmoHoldFreeplayEnd();
      }

      private void UnleashTheDoobersForOnboarding() {
        _OnboardingDooberStart.gameObject.SetActive(true);
        _OnboardingRewardTransforms1.Shuffle();
        _OnboardingRewardTransforms2.Shuffle();
        List<List<Transform>> allAreas = new List<List<Transform>>();
        allAreas.Add(_OnboardingRewardTransforms2);
        allAreas.Add(_OnboardingRewardTransforms1);
        Sequence dooberSequence = DOTween.Sequence();
        int whichItem = 0;
        foreach (string itemID in LootBoxRewards.Keys) {
          List<Transform> possibleSpots = allAreas[whichItem % allAreas.Count];
          for (int i = 0; i < LootBoxRewards[itemID]; i++) {
            Transform newDoob = SpawnDoober(itemID);
            Vector3 doobTarget = possibleSpots[i % possibleSpots.Count].position;
            dooberSequence.Join(newDoob.DOScale(0.0f, _RewardExplosionDuration).From().SetEase(Ease.OutBack));
            dooberSequence.Join(newDoob.DOMove(doobTarget, _RewardExplosionDuration).SetEase(Ease.OutBack));
          }
          whichItem++;
        }
        dooberSequence.Play();
        int numBits = 0;
        if (LootBoxRewards.ContainsKey(RewardedActionManager.Instance.CoinID)) {
          numBits = LootBoxRewards[RewardedActionManager.Instance.CoinID];
        }
        int numSparks = 0;
        if (LootBoxRewards.ContainsKey(RewardedActionManager.Instance.SparkID)) {
          numSparks = LootBoxRewards[RewardedActionManager.Instance.SparkID];
        }
        DAS.Event("onboarding.emotion_chip.open", numBits.ToString(), DASUtil.FormatExtraData(numSparks.ToString()));
        _ContinueButtonInstance.Initialize(OnboardingDooberSplosionComplete, "onboarding.button.loot", "Onboarding");

        ChestRewardManager.Instance.PendingChestRewards.Clear();
      }
      private void OnboardingDooberSplosionComplete() {
        _OnboardingDooberStart.gameObject.SetActive(false);
        ContextManager.Instance.CozmoHoldFreeplayEnd();
        CloseView();
      }

      private void TronLineBurst(int count) {
        for (int i = 0; i < count; i++) {
          _TronPool.GetObjectFromPool();
        }
      }

      // BEGONE DOOBERS! OUT OF MY SIGHT! Staggering their start times slightly, send all active doobers to the
      // FinalRewardTarget position
      private void SendDoobersAway(Sequence closeAnimation) {
        SendTransformsToFinalTarget(closeAnimation, _ActiveBitsTransforms, _FinalBitsTarget);
        SendTransformsToFinalTarget(closeAnimation, _ActiveSparkTransforms, _FinalSparkTarget);
      }

      private void SendTransformsToFinalTarget(Sequence currSequence, List<Transform> transList, Transform target) {
        Sequence dooberSequence = currSequence;
        for (int i = 0; i < transList.Count; i++) {
          Transform currDoob = transList[i];
          float doobStagger = UnityEngine.Random.Range(0, _RewardExplosionFinalVariance);
          dooberSequence.Insert(doobStagger, currDoob.DOScale(0.0f, _RewardExplosionFinalDuration).SetEase(Ease.InBack));
          dooberSequence.Insert(doobStagger, currDoob.DOMove(target.position, _RewardExplosionFinalDuration).SetEase(Ease.InBack));
        }

      }

      protected override void CleanUp() {
        _LootButton.onClick.RemoveAllListeners();
        _TronPool.ReturnAllObjectsToPool();
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          RobotEngineManager.Instance.CurrentRobot.SetAvailableGames(BehaviorGameFlag.All);
        }
        StopCoroutine(InitializeBox());
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
          (_BannerInstance.PlayBannerAnimation(Localization.Get(LocalizationKeys.kLootAnnounce), StartBoxAnimation)));

      }

      public void StartBoxAnimation() {
        _BoxSequence.TogglePause();
      }

      public void CreateBoxAnimation() {
        Sequence boxSequence = DOTween.Sequence();
        boxSequence.Append(_LootBox.DOScale(_BoxIntroStartScale, _BoxIntroTweenDuration).From().SetEase(Ease.InExpo));
        boxSequence.Join(_LootBox.DOMove(_BoxSource.position, _BoxIntroTweenDuration).From());
        boxSequence.Append(_LootBox.DOScale(_MinBoxScale, _BoxIntroSettleDuration).SetEase(Ease.InExpo));
        boxSequence.OnComplete(HandleBoxFinished);
        boxSequence.Play();
        boxSequence.TogglePause();
        _BoxSequence = boxSequence;
      }

      // Finish open animation, enable LootButton, create initial shake and flourish
      private void HandleBoxFinished() {
        _LootButton.onClick.AddListener(HandleButtonTap);
        _ShakeRotationTweener = _LootBox.DOShakeRotation(_ShakeDuration, new Vector3(0, 0, _ShakeRotationMaxAngle), _ShakeRotationVibrato, _ShakeRotationRandomness);
        _ShakePositionTweener = _LootBox.DOShakePosition(_ShakeDuration, _ShakePositionMax, _ShakePositionVibrato, _ShakePositionRandomness);
        _BurstParticles.Emit(_OpenChestBurst);
        TronLineBurst(_OpenChestBurst);
        _LootText.gameObject.SetActive(true);
        if (OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.Loot)) {
          _LootTextInstructions1.gameObject.SetActive(true);
          _LootTextInstructions2.gameObject.SetActive(true);
        }
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
