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

namespace Cozmo.UI {
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

    // How long the Reward animation takes to tween the rewards to their initial positions
    [SerializeField]
    private float _RewardExplosionDuration = 0.5f;
    // Variance for each Reward exploding from box to stagger them out more.
    [SerializeField]
    private float _RewardExplosionVariance = 0.25f;
    // How long the Rewards remain visible before leaving
    [SerializeField]
    private float _RewardExplosionStayDuration = 1.5f;
    // How long the Reward animation takes to tween the rewards to their final positions
    [SerializeField]
    private float _RewardExplosionFinalDuration = 1.25f;
    // The maximum amount of variance in seconds that rewards are randomly staggered by to create
    // less uniform movements
    [SerializeField]
    private float _RewardExplosionFinalVariance = 1.0f;

    [SerializeField]
    private float _RewardSpreadRadiusMax = 600.0f;
    [SerializeField]
    private float _RewardSpreadRadiusMin = 5.0f;
    // If a reward would be placed within spread proximity of an existing reward, instead attempt to place it in a different random
    // position to make the loot view look nicer. This dist is based on the last placed reward's sprite.
    private float _RewardSpreadMinDistance;
    // To prevent potential infinite loops or extremely inefficient sequences with large numbers of rewards, cap out spread attempts
    private const int kMaxSpreadAttempts = 6;
    // Keep track of positions we have already targeted for reward placement to check for spread
    private List<Vector3> _TargetedPositions = new List<Vector3>();

    #endregion

    #region Particle Update Settings

    // Tron Line settings
    [SerializeField]
    private int _TronBurstLow = 3;
    [SerializeField]
    private int _TronBurstMid = 5;
    [SerializeField]
    private int _TronBurstAlmost = 8;

    // Particle Burst Settings for when the box lands in place and is ready
    // as well as when it finally bursts open
    [SerializeField]
    private int _OpenChestBurst = 45;
    [SerializeField]
    private int _ReadyChestBurst = 10;

    [SerializeField, Tooltip("Charge % where TronBurst and Text go to Midtier")]
    private float _LootMidThreshold = 0.3f;
    [SerializeField, Tooltip("Charge % where TronBurst and Text go to almost tier")]
    private float _LootAlmostThreshold = 0.6f;

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

    private float _currentBoxCharge = 0.0f;
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
    private List<Transform> _ActiveBitsTransforms;
    private List<Transform> _ActiveSparkTransforms;
    [SerializeField]
    private Transform _RewardStartPosition;
    [SerializeField]
    private Transform _FinalBitsTarget;
    [SerializeField]
    private Transform _FinalSparkTarget;
    [SerializeField]
    private Transform _BoxSource;
    [SerializeField]
    private Transform _BannerContainer;
    [SerializeField]
    private GameObject _RewardParticlePrefab;
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
    private Image _LootFlash;

    [SerializeField]
    private Transform _OpenBox;

    private Tweener _ShakeRotationTweener;
    private Tweener _ShakePositionTweener;
    private Sequence _BoxSequence;
    private Sequence _RewardSequence;

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
    private Transform _OnboardingRewardStart;
    [SerializeField]
    private CozmoButton _ContinueButtonInstance;
    #endregion

    private bool _BoxOpened;

    private void Awake() {
      _OnboardingRewardStart.gameObject.SetActive(false);
      _ContinueButtonInstance.gameObject.SetActive(false);
      PauseManager.Instance.OnPauseDialogOpen += CloseView;
      _OpenBox.gameObject.SetActive(false);
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
      if (!_BoxOpened) {
        if (_currentBoxCharge >= 1.0f) {
          _BoxOpened = true;

          StopTweens();
          _LootBox.gameObject.SetActive(false);
          _OpenBox.gameObject.SetActive(true);
          _currentBoxCharge = 0.0f;
          _recentTapCharge = 1.0f;
          RewardLoot();
        }
        else {
          _currentBoxCharge += _ChargePerTap;
          _recentTapCharge = 1.0f;
          StopTweens();

          float currShake = Mathf.Lerp(_ShakeRotationMinAngle, _ShakeRotationMaxAngle, _currentBoxCharge);
          _ShakeRotationTweener = _LootBox.DOShakeRotation(_ShakeDuration, new Vector3(0, 0, currShake), _ShakeRotationVibrato, _ShakeRotationRandomness);
          currShake = Mathf.Lerp(_ShakePositionMin, _ShakePositionMax, _currentBoxCharge);
          _ShakePositionTweener = _LootBox.DOShakePosition(_ShakeDuration, currShake, _ShakePositionVibrato, _ShakePositionRandomness);
          Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_TapSoundEvent);

        }
      }
    }

    protected override void Update() {
      base.Update();

      // Handle Recent Tap Decay
      if (_recentTapCharge > 0.0f) {
        _recentTapCharge -= _TapDecayRate;
        _recentTapCharge = Mathf.Max(_recentTapCharge, 0.0f);
      }

      // Handle Current Charge Decay
      if (_currentBoxCharge > 0.0f) {
        _currentBoxCharge -= _ChargeDecayRate;
        _currentBoxCharge = Mathf.Max(_currentBoxCharge, 0.0f);
        // If charge is not depleted, periodically fire Tronlines
        // at a predetermined interval
        if (Time.time - _UpdateTimeStamp > _ChargeVFXUpdateInterval) {
          _UpdateTimeStamp = Time.time;
          TronLineBurst(_tronBurst);
        }
      }

      // Update Loot Glows/Flash based on if the box is opened or not
      if (_recentTapCharge > 0.0f) {
        // Fade out LootFlash when the box is opened
        if (_BoxOpened) {
          _LootFlash.color = new Color(_LootFlash.color.r, _LootFlash.color.g, _LootFlash.color.b, _recentTapCharge);
        }
        // Adjust scale and fade out LootGlow when the box is closed
        else {
          _LootGlow.color = new Color(_LootGlow.color.r, _LootGlow.color.g, _LootGlow.color.b, _recentTapCharge);
          float newScale = Mathf.Lerp(_MinBoxScale, _MaxBoxScale, _recentTapCharge);
          _LootBox.localScale = new Vector3(newScale, newScale, 1.0f);
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
      else if (_currentBoxCharge >= _LootAlmostThreshold) {
        LootText = _LootAlmostKey;
        _tronBurst = _TronBurstAlmost;
      }
      else if (_currentBoxCharge >= _LootMidThreshold) {
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
        SpawnOnBoardingRewards();
      }
      else {
        SpawnRewards();
      }
    }

    // Attempt to get a Spread Position that is appropriately spaced away from others
    private Vector3 GetSpreadPos(Vector3 origPos) {
      Vector3 newPos = new Vector3();
      int coinFlipX = 1;
      int coinFlipY = 1;
      int attempts = 0;
      while (attempts <= kMaxSpreadAttempts) {
        if (UnityEngine.Random.Range(0.0f, 1.0f) > 0.5f) {
          coinFlipX = -1;
        }
        if (UnityEngine.Random.Range(0.0f, 1.0f) > 0.5f) {
          coinFlipY = -1;
        }
        newPos = new Vector3(origPos.x + (UnityEngine.Random.Range(_RewardSpreadRadiusMin, _RewardSpreadRadiusMax) * coinFlipX),
                             origPos.y + (UnityEngine.Random.Range(_RewardSpreadRadiusMin, _RewardSpreadRadiusMax) * coinFlipY), 0.0f);
        if (IsValidSpreadPos(newPos) || attempts == kMaxSpreadAttempts) {
          // Only add a position to the list of targeted positions if it is properly placed,
          // Otherwise it should already be covered by existing targeted positions and will
          // be a waste to check.
          _TargetedPositions.Add(newPos);
          return newPos;
        }
        else {
          attempts++;
        }
      }
      return newPos;
    }

    // Check through all the existing positions, if any of them are closer to the checkPos
    // than our desired minimum spread distance, then return false. Otherwise return true.
    private bool IsValidSpreadPos(Vector3 checkPos) {
      for (int i = 0; i < _TargetedPositions.Count; i++) {
        float distSq = (_TargetedPositions[i] - checkPos).sqrMagnitude;
        if (distSq < _RewardSpreadMinDistance * _RewardSpreadMinDistance) {
          return false;
        }
      }
      return true;
    }

    private Transform SpawnReward(string rewardID) {
      Transform newReward = UIManager.CreateUIElement(_RewardParticlePrefab.gameObject, _RewardStartPosition).transform;
      Sprite rewardIcon = null;
      ItemData iData = ItemDataConfig.GetData(rewardID);
      if (iData != null) {
        rewardIcon = iData.Icon;
      }

      if (rewardIcon != null) {
        newReward.GetComponent<Image>().overrideSprite = rewardIcon;
      }
      if (rewardID == RewardedActionManager.Instance.SparkID) {
        _ActiveSparkTransforms.Add(newReward);
      }
      else {
        _ActiveBitsTransforms.Add(newReward);
      }
      _RewardSpreadMinDistance = Mathf.Max(rewardIcon.rect.width, rewardIcon.rect.height);
      return newReward;
    }

    private void SpawnRewards() {
      Transform rewardSource = _RewardStartPosition;
      int rewardCount = 0;
      Sequence rewardSequence = DOTween.Sequence();
      foreach (string itemID in LootBoxRewards.Keys) {
        rewardCount += LootBoxRewards[itemID];
        for (int i = 0; i < LootBoxRewards[itemID]; i++) {
          Transform newReward = SpawnReward(itemID);
          Vector3 rewardTarget = GetSpreadPos(rewardSource.position);
          float stagger = UnityEngine.Random.Range(0.0f, _RewardExplosionVariance);
          rewardSequence.Insert(stagger, newReward.DOScale(0.0f, _RewardExplosionDuration).From().SetEase(Ease.OutBack));
          rewardSequence.Join(newReward.DOLocalMove(rewardTarget, _RewardExplosionDuration).SetEase(Ease.OutBack));
        }
      }

      rewardSequence.InsertCallback(_RewardExplosionDuration + _RewardExplosionVariance + _RewardExplosionStayDuration, () => {
        CloseView();
      });
      rewardSequence.Play();
      _RewardSequence = rewardSequence;

      ContextManager.Instance.CozmoHoldFreeplayEnd();
    }

    private void SpawnOnBoardingRewards() {
      _OnboardingRewardStart.gameObject.SetActive(true);
      _OnboardingRewardTransforms1.Shuffle();
      _OnboardingRewardTransforms2.Shuffle();
      List<List<Transform>> allAreas = new List<List<Transform>>();
      allAreas.Add(_OnboardingRewardTransforms2);
      allAreas.Add(_OnboardingRewardTransforms1);
      Sequence rewardSequence = DOTween.Sequence();
      int whichItem = 0;
      foreach (string itemID in LootBoxRewards.Keys) {
        List<Transform> possibleSpots = allAreas[whichItem % allAreas.Count];
        for (int i = 0; i < LootBoxRewards[itemID]; i++) {
          Transform newReward = SpawnReward(itemID);
          Vector3 rewardTarget = possibleSpots[i % possibleSpots.Count].position;
          rewardSequence.Join(newReward.DOScale(0.0f, _RewardExplosionDuration).From().SetEase(Ease.OutBack));
          rewardSequence.Join(newReward.DOMove(rewardTarget, _RewardExplosionDuration).SetEase(Ease.OutBack));
        }
        whichItem++;
      }

      rewardSequence.Play();

      int numBits = 0;
      if (LootBoxRewards.ContainsKey(RewardedActionManager.Instance.CoinID)) {
        numBits = LootBoxRewards[RewardedActionManager.Instance.CoinID];
      }
      int numSparks = 0;
      if (LootBoxRewards.ContainsKey(RewardedActionManager.Instance.SparkID)) {
        numSparks = LootBoxRewards[RewardedActionManager.Instance.SparkID];
      }
      DAS.Event("onboarding.emotion_chip.open", numBits.ToString(), DASUtil.FormatExtraData(numSparks.ToString()));

      _ContinueButtonInstance.gameObject.SetActive(true);
      _ContinueButtonInstance.Initialize(HandleOnboardingRewardsContinueButton, "onboarding.button.loot", "Onboarding");

    }

    private void HandleOnboardingRewardsContinueButton() {
      _OnboardingRewardStart.gameObject.SetActive(false);
      _ContinueButtonInstance.gameObject.SetActive(false);
      ContextManager.Instance.CozmoHoldFreeplayEnd();
      CloseView();
    }

    private void TronLineBurst(int count) {
      for (int i = 0; i < count; i++) {
        _TronPool.GetObjectFromPool();
      }
    }

    private void AnimateRewardsToTarget(Sequence closeAnimation) {
      SendTransformsToFinalTarget(closeAnimation, _ActiveBitsTransforms, _FinalBitsTarget);
      SendTransformsToFinalTarget(closeAnimation, _ActiveSparkTransforms, _FinalSparkTarget);
    }

    private void SendTransformsToFinalTarget(Sequence currSequence, List<Transform> transList, Transform target) {
      Sequence rewardSequence = currSequence;
      for (int i = 0; i < transList.Count; i++) {
        Transform currentReward = transList[i];
        float rewardStartVariance = UnityEngine.Random.Range(0, _RewardExplosionFinalVariance);
        rewardSequence.Insert(rewardStartVariance, currentReward.DOScale(0.0f, _RewardExplosionFinalDuration).SetEase(Ease.InBack));
        rewardSequence.Insert(rewardStartVariance, currentReward.DOMove(target.position, _RewardExplosionFinalDuration).SetEase(Ease.InBack));
      }

    }

    protected override void CleanUp() {
      if (_RewardSequence != null) {
        _RewardSequence.Kill();
      }
      PauseManager.Instance.OnPauseDialogOpen -= CloseView;
      ChestRewardManager.Instance.ApplyChestRewards();
      RewardedActionManager.Instance.SendPendingRewardsToInventory();
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
        _ShakePositionTweener.Kill();
      }
      if (_ShakeRotationTweener != null) {
        _ShakeRotationTweener.Kill();
      }
      if (_BoxSequence != null) {
        _BoxSequence.Kill();
      }
    }

    protected override void ConstructOpenAnimation(Sequence openAnimation) {
      if (_AlphaController != null) {
        _AlphaController.alpha = 0;
        openAnimation.Join(_AlphaController.DOFade(1, 0.25f).SetEase(Ease.OutQuad));
      }
      openAnimation.OnComplete(() => {
        _BannerInstance.PlayBannerAnimation(Localization.Get(LocalizationKeys.kLootAnnounce),
      _BoxSequence.TogglePause);
      });
    }

    public void CreateBoxAnimation() {
      Sequence boxSequence = DOTween.Sequence();
      boxSequence.Append(_LootBox.DOScale(_BoxIntroStartScale, _BoxIntroTweenDuration).From().SetEase(Ease.InExpo));
      boxSequence.Join(_LootBox.DOMove(_BoxSource.position, _BoxIntroTweenDuration).From());
      boxSequence.Append(_LootBox.DOScale(_MaxBoxScale, _BoxIntroSettleDuration));
      boxSequence.Append(_LootBox.DOScale(_MinBoxScale, _BoxIntroSettleDuration).SetEase(Ease.InExpo));
      boxSequence.OnComplete(HandleBoxFinished);
      boxSequence.Play();
      boxSequence.TogglePause();
      _BoxSequence = boxSequence;
    }

    // Finish open animation, enable LootButton, create initial shake and flourish
    private void HandleBoxFinished() {
      UIManager.EnableTouchEvents();
      _LootButton.onClick.AddListener(HandleButtonTap);
      _ShakeRotationTweener = _LootBox.DOShakeRotation(_ShakeDuration, new Vector3(0, 0, _ShakeRotationMaxAngle), _ShakeRotationVibrato, _ShakeRotationRandomness);
      _ShakePositionTweener = _LootBox.DOShakePosition(_ShakeDuration, _ShakePositionMax, _ShakePositionVibrato, _ShakePositionRandomness);
      TronLineBurst(_ReadyChestBurst);
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
      AnimateRewardsToTarget(closeAnimation);
    }
  }
}
