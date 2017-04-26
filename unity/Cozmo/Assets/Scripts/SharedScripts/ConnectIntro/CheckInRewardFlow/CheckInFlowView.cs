using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;
using System.Collections;
using System.Collections.Generic;
using Cozmo.UI;
using DataPersistence;

namespace Cozmo.CheckInFlow.UI {
  public class CheckInFlowView : BaseView {

    private const int _kMaxTimelineCells = 7;

    public System.Action ConnectionFlowComplete;
    public System.Action CheckInFlowQuit;

    // Audio Config
    [SerializeField]
    private Anki.Cozmo.Audio.AudioEventParameter _EnvelopeOpenSound = Anki.Cozmo.Audio.AudioEventParameter.InvalidEvent;
    [SerializeField]
    private Anki.Cozmo.Audio.AudioEventParameter _EnvelopeDisappearSound = Anki.Cozmo.Audio.AudioEventParameter.InvalidEvent;
    [SerializeField]
    private Anki.Cozmo.Audio.AudioEventParameter _CalendarDisappearSound = Anki.Cozmo.Audio.AudioEventParameter.InvalidEvent;
    [SerializeField]
    private Anki.Cozmo.Audio.AudioEventParameter _GoalCollectSound = Anki.Cozmo.Audio.AudioEventParameter.InvalidEvent;
    [SerializeField]
    private Anki.Cozmo.Audio.AudioEventParameter _RewardCollectSound = Anki.Cozmo.Audio.AudioEventParameter.InvalidEvent;

    [SerializeField]
    private Cozmo.UI.CozmoButtonLegacy _EnvelopeButton;

    [SerializeField]
    private GameObject _EnvelopeContainer;

    [SerializeField]
    private Animator _EnvelopeAnimator;

    [SerializeField]
    private CheckInFlowEnvelopeOpenEvent _EnvelopeOpenEventScript;

    [SerializeField]
    private Transform _CheckInRewardIconContainer;
    [SerializeField]
    private Transform _CheckInRewardSpawnPoint;

    [SerializeField]
    private CheckInFlowRewardSpawnPoint _DailyGoalIconSpawnLayoutElement;
    [SerializeField]
    private CheckInFlowRewardSpawnPoint _EnergyRewardSpawnLayoutElement;
    [SerializeField]
    private CheckInFlowRewardSpawnPoint _BitRewardSpawnLayoutElement;
    [SerializeField]
    private CheckInFlowRewardSpawnPoint _SparkRewardSpawnLayoutElement;

    [SerializeField]
    private GameObject _CheckInFlowRewardPrefab;

    [SerializeField]
    private GameObject _TimelineReviewContainer;
    [SerializeField]
    private Animator _TimelineAnimator;
    [SerializeField]
    private HorizontalLayoutGroup _TimelineCellLayoutGroup;
    [SerializeField]
    private CheckInTimelineCell _CheckInTimelineCellPrefab;

    [SerializeField]
    private CheckInFlowRewardSpawnPoint _EnergyRewardCollectLayoutElement;
    [SerializeField]
    private CheckInFlowRewardSpawnPoint _BitRewardCollectLayoutElement;
    [SerializeField]
    private CheckInFlowRewardSpawnPoint _SparkRewardCollectLayoutElement;

    [SerializeField]
    private DailyGoalPanel _DailyGoalPanelScript;
    [SerializeField]
    private CanvasGroup _DailyGoalPanel;

    [SerializeField]
    private ProgressBar _EnergyChestBar;
    [SerializeField]
    private Text _EnergyChestBarText;

    [SerializeField]
    private GameObject _ConnectContainer;
    [SerializeField]
    private CanvasGroup _ConnectCanvas;
    [SerializeField]
    private Cozmo.UI.CozmoButtonLegacy _ConnectButton;

    [SerializeField]
    private Transform _FinalExpTarget;
    [SerializeField]
    private Transform _FinalCoinTarget;
    [SerializeField]
    private Transform _FinalSparkTarget;

    [SerializeField]
    private ConnectionFlowController _ConnectionFlowPrefab;
    private ConnectionFlowController _ConnectionFlowInstance;

    private List<CheckInTimelineCell> _CheckInTimelineCells = new List<CheckInTimelineCell>();

    private List<CheckInFlowReward> _DailyGoalIconDoobers = new List<CheckInFlowReward>();
    private List<GoalCell> _DailyGoalCells = new List<GoalCell>();

    private List<CheckInFlowReward> _EnergyRewardDoobers = new List<CheckInFlowReward>();
    private List<CheckInFlowReward> _BitRewardDoobers = new List<CheckInFlowReward>();
    private List<CheckInFlowReward> _SparkRewardDoobers = new List<CheckInFlowReward>();

    private Sequence _RewardSequence;
    private Sequence _MainUISequence;

    private void Awake() {
      _EnvelopeContainer.SetActive(false);
      _EnvelopeButton.gameObject.SetActive(false);
      _TimelineReviewContainer.SetActive(false);
      _ConnectContainer.SetActive(false);

      _ConnectCanvas.alpha = 0f;

      _DailyGoalPanelScript.ShowCompletedText(false);
      _DailyGoalPanel.gameObject.SetActive(true);
      _DailyGoalPanel.alpha = 0.0f;
      UIManager.Instance.BackgroundColorController.SetBackgroundColor(Cozmo.UI.BackgroundColorController.BackgroundColor.TintMe, Color.white);

      InitializeButtons();

      // automatically apply chest rewards that are queued up incase the app is exitied during the middle
      // of a reward loot view flow.
      ChestRewardManager.Instance.TryPopulateChestRewards();
      ChestRewardManager.Instance.ApplyChestRewards();
      UpdateEnergyProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints(), true);


      // SDK users always should skip...
      if (DataPersistence.DataPersistenceManager.Instance.Data.DeviceSettings.IsSDKEnabled) {
        HandleConnectButton();
      }
      // Do Check in Rewards if we need a new session
      else if (DataPersistence.DataPersistenceManager.Instance.IsNewSessionNeeded) {
        ShowCheckInRewardFlowUI();
      }
      else {
        ShowNormalConnectFlowUI();
      }
    }

    private void InitializeButtons() {
      _EnvelopeButton.Initialize(HandleEnvelopeButton, "envelope_button", "checkin_dialog");
      _EnvelopeButton.Text = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName;

      if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
        _ConnectButton.Initialize(HandleMockButton, "connect_button", "checkin_dialog");
      }
      else {
        _ConnectButton.Initialize(HandleConnectButton, "connect_button", "checkin_dialog");
      }
      _ConnectButton.Text = Localization.Get(LocalizationKeys.kLabelConnect);
    }

    private void UpdateEnergyProgressBar(int currPoints, int reqPoints, bool instant) {
      // If we've closed out of CheckInFlow midway through a tween, avoid potential null ref
      if (_EnergyChestBar == null) {
        return;
      }
      float prog = ((float)currPoints / (float)reqPoints);
      if (instant) {
        _EnergyChestBar.SetValueInstant(prog);
      }
      else {
        _EnergyChestBar.SetTargetAndAnimate(prog);
      }
      _EnergyChestBarText.text = Localization.GetNumber(currPoints);
    }

    private void ShowCheckInRewardFlowUI() {
      _EnvelopeContainer.SetActive(true);
      _EnvelopeButton.gameObject.SetActive(true);
    }

    private void ShowNormalConnectFlowUI() {
      Sequence panelSequence = DOTween.Sequence();
      panelSequence.Join(UIDefaultTransitionSettings.Instance.CreateFadeInTween(_DailyGoalPanel, Ease.Unset, _DailyGoalFadeInDuration_sec));
      panelSequence.Join(UIDefaultTransitionSettings.Instance.CreateFadeInTween(_ConnectCanvas, Ease.Unset, _DailyGoalFadeInDuration_sec));
      _ConnectContainer.SetActive(true);
      SetMainUISequence(panelSequence);
    }

    private void OnDestroy() {
      _EnvelopeOpenEventScript.OnEnvelopeOpen -= HandleEnvelopeOpenedEvent;
      _EnvelopeOpenEventScript.OnEnvelopeStartExit -= HandleEnvelopeStartExitEvent;
      if (_RewardSequence != null) {
        _RewardSequence.Kill();
      }
      if (_MainUISequence != null) {
        _MainUISequence.Kill();
      }
      if (_ConnectionFlowInstance != null) {
        GameObject.Destroy(_ConnectionFlowInstance.gameObject);
      }
      DeregisterOpenEnvelopeAnimationEvents();
      DeregisterTimelineAnimationEvents();
    }

    #region EnvelopeContainer

    // On tap play animation (crunch down slightly, then pop up and switch to open sprite)
    // On open, release rewards and transition to TimelineReviewContainer.
    private void HandleEnvelopeButton() {
      DAS.Event("meta.open_goal_envelope", DataPersistenceManager.Instance.Data.DefaultProfile.TotalSessions.ToString());

      _EnvelopeButton.Interactable = false;

      // Listen for end of open event to play timeline animation
      RegisterOpenEnvelopeAnimationEvents();

      // Play open envelope animation
      _EnvelopeAnimator.SetBool("PlayOpenAnimation", true);
      _EnvelopeOpenEventScript.OnEnvelopeOpen += HandleEnvelopeOpenedEvent;
      _EnvelopeOpenEventScript.OnEnvelopeStartExit += HandleEnvelopeStartExitEvent;
    }

    public void HandleEnvelopeOpenedEvent() {
      _EnvelopeOpenEventScript.OnEnvelopeOpen -= HandleEnvelopeOpenedEvent;
      // play the envelope open sound
      Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_EnvelopeOpenSound);
      PlaySpawnRewardsAnimation();
    }

    public void HandleEnvelopeStartExitEvent() {
      _EnvelopeOpenEventScript.OnEnvelopeStartExit -= HandleEnvelopeStartExitEvent;
      // play the envelope disappear sound
      Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_EnvelopeDisappearSound);
    }

    private void RegisterOpenEnvelopeAnimationEvents() {
      _EnvelopeAnimator.ListenForAnimationEnd(HandleOpenEnvelopeAnimationExitEvents);
    }

    private void DeregisterOpenEnvelopeAnimationEvents() {
      _EnvelopeAnimator.StopListenForAnimationEnd(HandleOpenEnvelopeAnimationExitEvents);
    }

    private void HandleOpenEnvelopeAnimationExitEvents(AnimatorStateInfo stateInfo) {
      if (stateInfo.IsName("EnvelopeExitAnimation")) {
        DeregisterOpenEnvelopeAnimationEvents();
        _EnvelopeButton.gameObject.SetActive(false);

        bool playerHasStreak = (DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.CurrentStreak > 1);
        if (playerHasStreak) {
          PlayTimelineEnterAnimation();
        }
        else {
          HandleAllStreakAnimationEnd();
        }
      }
    }

    #endregion

    #region Reward Spawn animation

    private bool _QuickConnect = true;

    [SerializeField]
    private float _InitialRewardIconScale = 0.5f;

    // Part of rewards includes generating new daily goals and animating the daily goal
    // panel into place
    // Add rewards to profile and  and auto advance.
    private void PlaySpawnRewardsAnimation() {
      // Start the new session as late as possible to allow for unlockIDs to load
      // Create the new session here to minimize risk of cheating/weird timing bugs allowing you to harvest
      // streak rewards by quitting out mid animation
      DataPersistence.DataPersistenceManager.Instance.StartNewSession();
      _QuickConnect = false;
      SpawnRewardIcons();
    }

    private void SpawnRewardIcons() {
      // Set up layout elements to provide tween targets for rewards.
      List<DailyGoal> currentGoals = DataPersistenceManager.Instance.CurrentSession.DailyGoals;
      Dictionary<RewardedActionData, int> rewardsToCreate = RewardedActionManager.Instance.PendingActionRewards;
      SetupSpawnLayoutElements(currentGoals.Count, rewardsToCreate);

      StartCoroutine(DelayedSpawnRewardIcons());
    }

    private void SetupSpawnLayoutElements(int currentNumGoals, Dictionary<RewardedActionData, int> rewardsToCreate) {
      _DailyGoalIconSpawnLayoutElement.gameObject.SetActive(currentNumGoals > 0);

      bool isRewardingEnergy = false;
      bool isRewardingBits = false;
      bool isRewardingSparks = false;
      string itemID = "";
      foreach (RewardedActionData reward in rewardsToCreate.Keys) {
        itemID = reward.Reward.ItemID;
        if (itemID == RewardedActionManager.Instance.EnergyID) {
          isRewardingEnergy = true;
        }
        else if (itemID == RewardedActionManager.Instance.CoinID) {
          isRewardingBits = true;
        }
        else if (itemID == RewardedActionManager.Instance.SparkID) {
          isRewardingSparks = true;
        }
        else {
          DAS.Warn("CheckInFlowDialog.SpawnRewardIcons.InvalidItemID", "Trying to spawn reward for unsupported item! itemID=" + itemID);
        }
      }

      _EnergyRewardSpawnLayoutElement.gameObject.SetActive(isRewardingEnergy);
      _EnergyRewardCollectLayoutElement.gameObject.SetActive(isRewardingEnergy);
      _BitRewardSpawnLayoutElement.gameObject.SetActive(isRewardingBits);
      _BitRewardCollectLayoutElement.gameObject.SetActive(isRewardingBits);
      _SparkRewardSpawnLayoutElement.gameObject.SetActive(isRewardingSparks);
      _SparkRewardCollectLayoutElement.gameObject.SetActive(isRewardingSparks);
    }

    private IEnumerator DelayedSpawnRewardIcons() {
      yield return null;

      // Spawn all the icons
      CreateDailyGoalRewardDoobers();
      CreateItemRewardDoobers();

      // Tween all the icons
      float dooberDelay_sec = 0f;
      Sequence spawnRewardSequence = DOTween.Sequence();
      TweenDoobersForSpawning(_DailyGoalIconDoobers, _DailyGoalIconSpawnLayoutElement, spawnRewardSequence, ref dooberDelay_sec);
      TweenDoobersForSpawning(_EnergyRewardDoobers, _EnergyRewardSpawnLayoutElement, spawnRewardSequence, ref dooberDelay_sec);
      TweenDoobersForSpawning(_BitRewardDoobers, _BitRewardSpawnLayoutElement, spawnRewardSequence, ref dooberDelay_sec);
      TweenDoobersForSpawning(_SparkRewardDoobers, _SparkRewardSpawnLayoutElement, spawnRewardSequence, ref dooberDelay_sec);
      SetRewardSequence(spawnRewardSequence);
    }

    #region Reward spawning

    // Spawn Daily Goal rewards for Goal Cell animations
    private void CreateDailyGoalRewardDoobers() {
      CheckInFlowReward newReward = null;
      GoalCell goalCell;
      Sprite dailyGoalIcon = DailyGoalManager.Instance.GetDailyGoalGenConfig().DailyGoalIcon;
      DailyGoalPanel dailyGoalPanel = DailyGoalManager.Instance.GoalPanelInstance;
      List<DailyGoal> currentGoals = DataPersistenceManager.Instance.CurrentSession.DailyGoals;
      for (int i = 0; i < currentGoals.Count; i++) {
        newReward = SpawnRewardIcon(dailyGoalIcon, 0);
        _DailyGoalIconDoobers.Add(newReward);

        goalCell = dailyGoalPanel.GetGoalSource(currentGoals[i]);
        goalCell.CanvasGroup.alpha = 0f;
        _DailyGoalCells.Add(goalCell);
      }
    }

    private void CreateItemRewardDoobers() {
      CheckInFlowReward newReward;
      Sprite itemIcon;
      string itemID = "";
      Dictionary<RewardedActionData, int> rewardsToCreate = RewardedActionManager.Instance.PendingActionRewards;
      List<CheckInFlowReward> rewardsWithCounts = new List<CheckInFlowReward>();
      foreach (RewardedActionData reward in rewardsToCreate.Keys) {
        itemID = reward.Reward.ItemID;
        DAS.Event("meta.open_goal_envelope_reward", itemID, DASUtil.FormatExtraData(reward.Reward.Amount.ToString()));
        itemIcon = GetSpriteFromItemId(itemID);
        if (itemID == RewardedActionManager.Instance.EnergyID) {
          newReward = SpawnRewardIcon(itemIcon, reward.Reward.Amount);
          AddToDooberList(itemID, newReward);
          rewardsWithCounts.Add(newReward);
        }
        else {
          // Instead of doing a count, do a cluster with non exp rewards
          for (int i = 0; i < reward.Reward.Amount; i++) {
            // Pass in 0 to hide label
            newReward = SpawnRewardIcon(itemIcon, 0);
            AddToDooberList(itemID, newReward);
          }
        }
      }

      // Make sure text appears above other icons
      foreach (CheckInFlowReward reward in rewardsWithCounts) {
        reward.transform.SetAsLastSibling();
      }
    }

    private Sprite GetSpriteFromItemId(string itemId) {
      Sprite itemIcon = null;
      // Initialize reward particles with appropriate values
      ItemData iData = ItemDataConfig.GetData(itemId);
      if (iData != null) {
        itemIcon = iData.Icon;
      }
      return itemIcon;
    }

    private CheckInFlowReward SpawnRewardIcon(Sprite sprite, int count) {
      Transform newReward = UIManager.CreateUIElement(_CheckInFlowRewardPrefab.gameObject, _CheckInRewardIconContainer).transform;
      newReward.localPosition = _CheckInRewardSpawnPoint.localPosition;
      newReward.localScale = new Vector3(_InitialRewardIconScale, _InitialRewardIconScale, _InitialRewardIconScale);

      CheckInFlowReward rewardScript = newReward.GetComponent<CheckInFlowReward>();
      rewardScript.RewardIcon.sprite = sprite;
      rewardScript.Alpha = 0f;
      if (count == 0) {
        rewardScript.RewardCount.gameObject.SetActive(false);
      }
      else {
        rewardScript.RewardCount.text = Localization.GetWithArgs(LocalizationKeys.kLabelPlusCount,
                                                                 Localization.GetNumber(count));
      }
      return rewardScript;
    }

    private void AddToDooberList(string itemId, CheckInFlowReward newReward) {
      if (itemId == RewardedActionManager.Instance.CoinID) {
        _BitRewardDoobers.Add(newReward);
      }
      else if (itemId == RewardedActionManager.Instance.SparkID) {
        _SparkRewardDoobers.Add(newReward);
      }
      else if (itemId == RewardedActionManager.Instance.EnergyID) {
        _EnergyRewardDoobers.Add(newReward);
      }
      else {
        DAS.Warn("CheckInFlowDialog.AddToDooberList.InvalidItemID", "Trying to spawn reward for unsupported item! itemID=" + itemId);
      }
    }

    #endregion

    #region Reward tweening

    [SerializeField]
    private float _DooberStagger_sec = 0.1f;

    [SerializeField]
    private float _DooberFadeInDuration_sec = 0.25f;

    [SerializeField]
    private float _DooberArcMovementDuration_sec = 0.5f;

    private void TweenDoobersForSpawning(List<CheckInFlowReward> doobersToTween, CheckInFlowRewardSpawnPoint targetArea, Sequence rewardSequence, ref float tweenDelay_sec) {
      foreach (CheckInFlowReward doober in doobersToTween) {
        doober.TargetSpawnRestPosition = null;
      }
      foreach (CheckInFlowReward doober in doobersToTween) {
        // Find a spawn target that won't be covered by the other rewards
        Vector3 dooberTarget = FindEmptyPositionNear(doober, doobersToTween, targetArea);
        doober.TargetSpawnRestPosition = dooberTarget;
        CreateSpawnTween(doober, dooberTarget, tweenDelay_sec, rewardSequence);
        tweenDelay_sec += _DooberStagger_sec;
      }
    }

    private Vector3 FindEmptyPositionNear(CheckInFlowReward currentDoober, List<CheckInFlowReward> otherDoobers, CheckInFlowRewardSpawnPoint targetArea) {
      int maximumAttempts = 10;
      float outerRange = (targetArea.Radius_px - currentDoober.Radius_px);

      Vector2 randomPosition;
      bool doesCollide = false;
      Vector3 newPosition = Vector3.zero;
      for (int i = 0; i < maximumAttempts; i++) {
        doesCollide = false;
        randomPosition = Random.insideUnitCircle * outerRange;
        newPosition.x = randomPosition.x + targetArea.transform.localPosition.x;
        newPosition.y = randomPosition.y + targetArea.transform.localPosition.y;

        Vector3 otherPosition;
        float minimumDistance;
        foreach (CheckInFlowReward otherDoober in otherDoobers) {
          if (otherDoober.TargetSpawnRestPosition.HasValue) {
            otherPosition = otherDoober.TargetSpawnRestPosition.Value;
            minimumDistance = (otherDoober.Radius_px + currentDoober.Radius_px);
            if ((otherPosition - newPosition).sqrMagnitude < (minimumDistance * minimumDistance)) {
              doesCollide = true;
              break;
            }
          }
        }

        if (!doesCollide) {
          break;
        }
      }
      return newPosition;
    }

    private void CreateSpawnTween(CheckInFlowReward dooberToTween, Vector3 movementTarget, float delay_sec, Sequence rewardSequence) {
      // Delay by X seconds
      // Insert Tween to target x location (linear)
      rewardSequence.Insert(delay_sec, dooberToTween.transform.DOLocalMoveX(movementTarget.x, _DooberArcMovementDuration_sec));
      // Join Tween to target y location (out back)
      rewardSequence.Join(dooberToTween.transform.DOLocalMoveY(movementTarget.y, _DooberArcMovementDuration_sec).SetEase(Ease.OutBack));
      // Append Tween to 1 scale (in out back)
      rewardSequence.Join(dooberToTween.transform.DOScale(1f, _DooberArcMovementDuration_sec).SetEase(Ease.OutBack));
      // Join Tween to 1 alpha, shorter duration
      rewardSequence.Join(dooberToTween.RewardIcon.DOFade(1, _DooberFadeInDuration_sec).SetEase(Ease.OutQuad));
    }

    #endregion

    #endregion

    #region Timeline Enter

    [SerializeField, Tooltip("Seconds to wait before tweening in the timeline after the envelope starts to exit.")]
    private float _TimelineStaggerDuration_sec = 0.25f;

    private int _IndexOfLastStreakDayTimelineCell = 0;

    private void PlayTimelineEnterAnimation() {
      CreateTimelineCells();

      Sequence timelineSequence = DOTween.Sequence();
      // Fade in the Timeline as we move the current envelope to the respective location, then hide it and replace it with its own StreakEnvelopeCell
      timelineSequence.AppendInterval(_TimelineStaggerDuration_sec);
      timelineSequence.AppendCallback(() => {
        _TimelineReviewContainer.SetActive(true);
        _TimelineAnimator.SetBool("PlayEnterAnimation", true);
        RegisterTimelineAnimationEvents();
      });

      SetMainUISequence(timelineSequence);
    }

    private void CreateTimelineCells() {
      List<CheckInTimelineCell> newCells = new List<CheckInTimelineCell>();
      for (int i = 0; i < _kMaxTimelineCells; i++) {
        CheckInTimelineCell newCell = UIManager.CreateUIElement(_CheckInTimelineCellPrefab, _TimelineCellLayoutGroup.transform).GetComponent<CheckInTimelineCell>();
        newCells.Add(newCell);
      }
      _CheckInTimelineCells.AddRange(newCells);

      int currentStreakDays = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.CurrentStreak;
      int firstTimelineCell = 1;
      if (currentStreakDays > _kMaxTimelineCells) {
        // Offset First Day based on max streak cells to fix off by 1 bug
        firstTimelineCell = (currentStreakDays - _kMaxTimelineCells) + 1;
      }

      int currDay = firstTimelineCell;
      bool streakMet, penultimateDay, finalDay;
      for (int i = 0; i < _kMaxTimelineCells; i++) {
        streakMet = (currDay <= currentStreakDays);
        penultimateDay = (currDay == (currentStreakDays - 1));
        finalDay = (currDay == currentStreakDays);
        _CheckInTimelineCells[i].InitializeCell(currDay, streakMet, penultimateDay, finalDay, i);
        if (streakMet) {
          _CheckInTimelineCells[i].OnAnimationComplete += HandleStreakAnimComplete;
        }
        if (finalDay) {
          _IndexOfLastStreakDayTimelineCell = i;
        }
        currDay++;
      }
    }

    private void RegisterTimelineAnimationEvents() {
      _TimelineAnimator.ListenForAnimationEnd(HandleTimelineAnimationExitEvents);
    }

    private void DeregisterTimelineAnimationEvents() {
      _TimelineAnimator.StopListenForAnimationEnd(HandleTimelineAnimationExitEvents);
    }

    private void HandleTimelineAnimationExitEvents(AnimatorStateInfo animationState) {
      if (animationState.IsName("TimelineEnterAnimation")) {
        PlayEnvelopeCellStreakAnimation();
        DeregisterTimelineAnimationEvents();
      }
      else if (animationState.IsName("TimelineExitAnimation")) {
        DeregisterTimelineAnimationEvents();
        HandleAllStreakAnimationEnd();
      }
    }

    private void PlayEnvelopeCellStreakAnimation() {
      _CheckInTimelineCells[0].PlayAnimation();
    }

    /// <summary>
    /// Handles whenever each Streak Cell completes its animation/Prog bar fill.
    /// Either start the next one, or handle the StreakFillEnd.
    /// </summary>
    private void HandleStreakAnimComplete(int index) {
      if (index >= _IndexOfLastStreakDayTimelineCell) {
        HandleStreakFillEnd();
      }
      else {
        _CheckInTimelineCells[index + 1].PlayAnimation();
      }
    }

    private void HandleStreakFillEnd() {
      // Play timeline exit animation
      _TimelineAnimator.SetBool("PlayExitAnimation", true);
      RegisterTimelineAnimationEvents();

      // play the sound for the timeline disappearing
      Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_CalendarDisappearSound);
    }

    #endregion

    #region Move to Collect Zone animation

    [SerializeField]
    private float _DooberToCollectZoneTweenDuration_sec = 0.25f;
    [SerializeField]
    private float _DailyGoalFadeInDuration_sec = 1.5f;
    [SerializeField]
    private float _GoalTweenDelay = 0.05f;
    [SerializeField]
    private float _GoalDooberStagger_sec = 0.2f;
    [SerializeField]
    private float _GoalSendoffDuration_sec = 0.5f;
    [SerializeField]
    private float _GoalDooberFadeOutDuration_sec = 0.5f;
    [SerializeField]
    private float _GoalDooberFinalScale = 2f;
    [SerializeField]
    private float _GoalCellFadeInDuration_sec = 0.25f;
    [SerializeField]
    private float _ConnectButtonFadeDuration = 1.5f;

    private void HandleAllStreakAnimationEnd() {
      _ConnectContainer.SetActive(true);

      Sequence rewardSequence = DOTween.Sequence();
      rewardSequence.Join(UIDefaultTransitionSettings.Instance.CreateFadeInTween(_DailyGoalPanel, Ease.Unset, _DailyGoalFadeInDuration_sec));
      rewardSequence.InsertCallback(_GoalTweenDelay, HandleDailyGoalTweens);

      float delay_sec = 0f;
      MoveRewardsToCollectZone(rewardSequence, ref delay_sec);

      SetRewardSequence(rewardSequence);
    }

    private void MoveRewardsToCollectZone(Sequence goalSequence, ref float delay_sec) {
      // Tween Goals to Panel and other rewards to collect zone.
      TweenDoobersToCollectZone(_EnergyRewardDoobers, _EnergyRewardCollectLayoutElement, goalSequence, ref delay_sec);
      goalSequence.AppendCallback(() => {
        UpdateEnergyProgressBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints(), true);
      });
      TweenDoobersToCollectZone(_BitRewardDoobers, _BitRewardCollectLayoutElement, goalSequence, ref delay_sec);
      TweenDoobersToCollectZone(_SparkRewardDoobers, _SparkRewardCollectLayoutElement, goalSequence, ref delay_sec);
    }

    private void TweenDoobersToCollectZone(List<CheckInFlowReward> doobersToTween, CheckInFlowRewardSpawnPoint targetArea, Sequence rewardSequence, ref float tweenDelay_sec) {
      foreach (CheckInFlowReward dooberToClear in doobersToTween) {
        dooberToClear.TargetSpawnRestPosition = null;
      }

      CheckInFlowReward doober;
      Vector3 dooberTarget;
      for (int i = doobersToTween.Count - 1; i >= 0; i--) {
        // Find a spawn target that won't be covered by the other rewards
        doober = doobersToTween[i];
        dooberTarget = FindEmptyPositionNear(doober, doobersToTween, targetArea);
        doober.TargetSpawnRestPosition = dooberTarget;
        CreateToCollectZoneTween(doober, dooberTarget, tweenDelay_sec, rewardSequence);
      }
      tweenDelay_sec += _GoalDooberStagger_sec;
    }

    private void CreateToCollectZoneTween(CheckInFlowReward dooberToTween, Vector3 movementTarget, float delay_sec, Sequence rewardSequence) {
      // Delay by X seconds
      // Insert Tween to target x location (linear)
      rewardSequence.Insert(delay_sec, dooberToTween.transform.DOLocalMoveX(movementTarget.x, _DooberToCollectZoneTweenDuration_sec));
      // Join Tween to target y location (out back)
      rewardSequence.Join(dooberToTween.transform.DOLocalMoveY(movementTarget.y, _DooberToCollectZoneTweenDuration_sec).SetEase(Ease.InOutSine));
      // Append Tween to 1 scale (in out back)
    }

    // Handled as a callback in order for us to have the appropriate delay after DailyGoalPanel becomes
    // active for Start to fire.
    private void HandleDailyGoalTweens() {
      Sequence dailyGoalTransformSequence = DOTween.Sequence();

      // play the sound for the goals animating toward the goal panel
      dailyGoalTransformSequence.AppendCallback(() => {
        Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_GoalCollectSound);
      });

      CheckInFlowReward dailyGoalDoober;
      GoalCell dailyGoal;
      float delay_sec = 0f;
      for (int i = 0; i < _DailyGoalIconDoobers.Count; i++) {
        // Since Goal reward particles are made based on daily goal count, these lists should always 
        // be the same size and correspond 1 to 1
        dailyGoalDoober = _DailyGoalIconDoobers[i];
        dailyGoal = _DailyGoalCells[i];
        if (dailyGoal != null) {
          PlayComplexGoalTransformAnimation(dailyGoalDoober, dailyGoal, delay_sec, ref dailyGoalTransformSequence);
        }
        else {
          PlaySimpleGoalTransformAnimation(dailyGoalDoober, delay_sec, ref dailyGoalTransformSequence);
        }
        delay_sec += _DooberStagger_sec;
      }

      dailyGoalTransformSequence.AppendCallback(() => {
        for (int i = 0; i < _DailyGoalIconDoobers.Count; i++) {
          _DailyGoalIconDoobers[i].gameObject.SetActive(false);
        }
        _TimelineReviewContainer.SetActive(false);
      });
      dailyGoalTransformSequence.Append(UIDefaultTransitionSettings.Instance.CreateFadeInTween(_ConnectCanvas, Ease.Unset, _ConnectButtonFadeDuration));

      SetMainUISequence(dailyGoalTransformSequence);
    }

    private void PlayComplexGoalTransformAnimation(CheckInFlowReward dooberToTween, GoalCell targetDailyGoal, float delay_sec, ref Sequence sequenceToAppend) {
      Transform targetPosition = targetDailyGoal.GoalCellSource;
      sequenceToAppend.Insert(delay_sec, dooberToTween.transform.DOMoveX(targetPosition.position.x, _GoalSendoffDuration_sec));
      sequenceToAppend.Join(dooberToTween.transform.DOMoveY(targetPosition.position.y, _GoalSendoffDuration_sec).SetEase(Ease.OutQuad));
      sequenceToAppend.Append(dooberToTween.RewardIcon.DOFade(0.0f, _GoalDooberFadeOutDuration_sec));
      sequenceToAppend.Join(dooberToTween.transform.DOScale(_GoalDooberFinalScale, _GoalDooberFadeOutDuration_sec).SetEase(Ease.OutQuad));
      sequenceToAppend.Join(targetDailyGoal.CanvasGroup.DOFade(1f, _GoalCellFadeInDuration_sec).SetEase(Ease.OutQuad));
    }

    private void PlaySimpleGoalTransformAnimation(CheckInFlowReward dooberToTween, float delay_sec, ref Sequence sequenceToAppend) {
      sequenceToAppend.Insert(delay_sec, dooberToTween.RewardIcon.DOFade(0.0f, _GoalDooberFadeOutDuration_sec));
    }

    #endregion

    #region Collect Rewards animation
    [SerializeField]
    private float _RewardSendoffDuration_sec = 0.25f;
    [SerializeField]
    private float _RewardSendoffScaleFactor = 0.5f;
    [SerializeField]
    private float _RewardSendoffFadeOutDelay_sec = 0.25f;
    [SerializeField]
    private float _RewardSendoffEndWaitTime_sec = 0.75f;

    private void AnimateRewardsToTarget(Sequence goalSequence) {
      goalSequence.AppendCallback(RewardedActionManager.Instance.SendPendingRewardsToInventory);
      goalSequence.AppendCallback(() => {
        int finalCheckInTarget = ChestRewardManager.Instance.GetNextRequirementPoints();
        int finalCheckInPoints = Mathf.Min(ChestRewardManager.Instance.GetCurrentRequirementPoints(), finalCheckInTarget);
        UpdateEnergyProgressBar(finalCheckInPoints, finalCheckInTarget, false);
      });

      float animationStart_sec = 0f;
      goalSequence = TweenAllToFinalTarget(goalSequence, _EnergyRewardDoobers, _FinalExpTarget, ref animationStart_sec);
      goalSequence = TweenAllToFinalTarget(goalSequence, _BitRewardDoobers, _FinalCoinTarget, ref animationStart_sec);
      goalSequence = TweenAllToFinalTarget(goalSequence, _SparkRewardDoobers, _FinalSparkTarget, ref animationStart_sec);

      goalSequence.AppendInterval(_RewardSendoffEndWaitTime_sec);
    }

    private Sequence TweenAllToFinalTarget(Sequence currSequence, List<CheckInFlowReward> doobersToTween, Transform finalTarget, ref float animationStart_sec) {
      for (int i = doobersToTween.Count - 1; i >= 0; i--) {
        CheckInFlowReward reward = doobersToTween[i];
        Transform currTransform = reward.transform;
        currSequence.Insert(animationStart_sec, currTransform.DOMoveX(finalTarget.position.x, _RewardSendoffDuration_sec).SetEase(Ease.Linear));
        currSequence.Join(currTransform.DOMoveY(finalTarget.position.y, _RewardSendoffDuration_sec).SetEase(Ease.InQuad));
        currSequence.Join(currTransform.DOScale(_RewardSendoffScaleFactor, _RewardSendoffDuration_sec).SetEase(Ease.InQuad));
        currSequence.Join(reward.RewardIcon.DOFade(0f, _RewardSendoffDuration_sec).SetEase(Ease.OutQuad).SetDelay(_RewardSendoffFadeOutDelay_sec));
        if (reward.RewardCount.gameObject.activeInHierarchy) {
          currSequence.Join(reward.RewardCount.DOFade(0f, _RewardSendoffDuration_sec).SetEase(Ease.OutQuad).SetDelay(_RewardSendoffFadeOutDelay_sec));
        }

        animationStart_sec += _DooberStagger_sec;
      }
      return currSequence;
    }

    #endregion

    #region Connection Flow

    private void HandleMockButton() {
      if (_QuickConnect) {
        HandleMockConnect();
      }
      else {
        _ConnectButton.Interactable = false;
        Sequence collectSequence = DOTween.Sequence();
        AnimateRewardsToTarget(collectSequence);
        collectSequence.AppendCallback(HandleMockConnect);
        SetRewardSequence(collectSequence);
      }
    }

    private void HandleMockConnect() {
      RobotEngineManager.Instance.MockConnect();
      if (DataPersistence.DataPersistenceManager.Instance.IsNewSessionNeeded) {
        DataPersistence.DataPersistenceManager.Instance.StartNewSession();
      }
      if (ConnectionFlowComplete != null) {
        ConnectionFlowComplete();
      }
    }

    private void HandleConnectButton() {
      _ConnectButton.Interactable = false;
      if (_QuickConnect) {
        StartConnectionFlow();
      }
      else {
        Sequence collectSequence = DOTween.Sequence();
        // play collect sound
        collectSequence.InsertCallback(0, () => {
          Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_RewardCollectSound);
        });
        AnimateRewardsToTarget(collectSequence);
        collectSequence.AppendCallback(StartConnectionFlow);
        SetRewardSequence(collectSequence);
      }
    }

    private void StartConnectionFlow() {
      int daysWithCozmo = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.DaysWithCozmo;
      DAS.Event("meta.time_with_cozmo", daysWithCozmo.ToString());
      DAS.Event("meta.games_played", DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.TotalGamesPlayed.Count.ToString());
      DAS.Event("meta.current_streak", DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.CurrentStreak.ToString());

      _ConnectionFlowInstance = GameObject.Instantiate(_ConnectionFlowPrefab.gameObject).GetComponent<ConnectionFlowController>();
      _ConnectionFlowInstance.ConnectionFlowComplete += HandleConnectionFlowComplete;
      _ConnectionFlowInstance.ConnectionFlowQuit += HandleConnectionFlowQuit;
      _ConnectionFlowInstance.StartConnectionFlow();
    }

    private void HandleConnectionFlowComplete() {
      if (_ConnectionFlowInstance != null) {
        GameObject.Destroy(_ConnectionFlowInstance.gameObject);
      }
      if (ConnectionFlowComplete != null) {
        ConnectionFlowComplete();
      }
    }

    private void HandleConnectionFlowQuit() {
      if (_ConnectionFlowInstance != null) {
        GameObject.Destroy(_ConnectionFlowInstance.gameObject);
      }
      if (CheckInFlowQuit != null) {
        CheckInFlowQuit();
      }
    }

    public void HandleRobotDisconnect() {
      if (_ConnectionFlowInstance != null) {
        _ConnectionFlowInstance.HandleRobotDisconnect();
      }
    }

    #endregion

    private void SetRewardSequence(Sequence sequence) {
      if (_RewardSequence != null) {
        _RewardSequence.Kill();
      }
      _RewardSequence = sequence;
      _RewardSequence.Play();
    }

    private void SetMainUISequence(Sequence sequence) {
      if (_MainUISequence != null) {
        _MainUISequence.Kill();
      }
      _MainUISequence = sequence;
      _MainUISequence.Play();
    }
  }
}
