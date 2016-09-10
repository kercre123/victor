using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;
using System.Collections.Generic;
using Anki.Cozmo;
using Anki.UI;
using Cozmo;
using Cozmo.UI;
using DataPersistence;

public class CheckInFlow : MonoBehaviour {

  private const int kMaxStreakCells = 7;

  public System.Action ConnectionFlowComplete;
  public System.Action CheckInFlowQuit;

  [SerializeField]
  private Cozmo.UI.CozmoButton _EnvelopeButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _ConnectButton;

  [SerializeField]
  private GameObject _EnvelopeContainer;
  [SerializeField]
  private Text _TapToOpenText;

  [SerializeField]
  private GameObject _TimelineReviewContainer;
  [SerializeField]
  private CanvasGroup _TimelineCanvas;
  [SerializeField]
  private GameObject _OpenEnvelope;
  [SerializeField]
  private Text _OpenEnvelopeNameText;
  [SerializeField]
  private Transform _FinalEnvelopeTarget;
  [SerializeField]
  private Image _EnvelopeShadow;

  [SerializeField]
  private GameObject _ConnectContainer;
  [SerializeField]
  private CanvasGroup _ConnectCanvas;
  [SerializeField]
  private CanvasGroup _DailyGoalPanel;
  [SerializeField]
  private Transform _NewGoalsFirstTarget;
  [SerializeField]
  private Transform _NewGoalsFinalTarget;
  [SerializeField]
  private GameObject _RewardPrefab;
  [SerializeField]
  private GameObject _DayCellPrefab;
  [SerializeField]
  private HorizontalLayoutGroup _DayCellLayoutGroup;
  [SerializeField]
  private GameObject _ProgBarPrefab;
  [SerializeField]
  private HorizontalLayoutGroup _ProgBarLayoutGroup;
  [SerializeField]
  private List<StreakCell> _DayCellList = new List<StreakCell>();
  [SerializeField]
  private ProgressBar _GoalProgressBar;
  [SerializeField]
  private Text _GoalProgressText;

  [SerializeField]
  private ConnectionFlow _ConnectionFlowPrefab;
  private ConnectionFlow _ConnectionFlowInstance;

  [SerializeField]
  private List<Transform> _MultRewardsTransforms;
  [SerializeField]
  private Transform _RewardParticleSource;
  [SerializeField]
  private Transform _FinalExpTarget;
  [SerializeField]
  private Transform _FinalCoinTarget;
  [SerializeField]
  private Transform _FinalSparkTarget;
  [SerializeField]
  private Transform _GoalsCollectTarget;

  [SerializeField]
  private PrivacyPolicyView _PrivacyPolicyViewPrefab;

  [SerializeField]
  private Cozmo.UI.CozmoButton _PrivacyPolicyButton;

  private List<Transform> _ActiveNewGoalTransforms = new List<Transform>();
  private List<Transform> _ActiveExpTransforms = new List<Transform>();
  private List<Transform> _ActiveSparksTransforms = new List<Transform>();
  private List<Transform> _ActiveCoinsTransforms = new List<Transform>();

  private Sequence _TimelineSequence;
  private Sequence _CurrentSequence;

  private void Awake() {

    if (RobotEngineManager.Instance.RobotConnectionType == RobotEngineManager.ConnectionType.Mock) {
      _ConnectButton.Initialize(HandleMockButton, "connect_button", "checkin_dialog");
    }
    else {
      _ConnectButton.Initialize(HandleConnectButton, "connect_button", "checkin_dialog");
    }

    _PrivacyPolicyButton.Initialize(() => {
      UIManager.OpenView(_PrivacyPolicyViewPrefab);
    }, "privacy_policy_button", "checkin_dialog");


    _EnvelopeButton.Initialize(HandleEnvelopeButton, "envelope_button", "checkin_dialog");
    _EnvelopeButton.Text = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName;
    _OpenEnvelopeNameText.text = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile.ProfileName;
    _EnvelopeContainer.SetActive(false);
    _TimelineReviewContainer.SetActive(false);
    _ConnectContainer.SetActive(false);
    _DailyGoalPanel.gameObject.SetActive(false);
    // Do Check in Rewards if we need a new session
    if (DataPersistence.DataPersistenceManager.Instance.IsNewSessionNeeded) {
      _EnvelopeContainer.SetActive(true);
      UpdateProgBar(0, 100);
    }
    else {
      Sequence rewardSequence = DOTween.Sequence();
      _CurrentSequence = rewardSequence;
      rewardSequence.Join(UIDefaultTransitionSettings.Instance.CreateFadeInTween(_DailyGoalPanel, Ease.Unset, _ConnectIntroDuration));
      rewardSequence.Join(UIDefaultTransitionSettings.Instance.CreateFadeInTween(_ConnectCanvas, Ease.Unset, _ConnectIntroDuration));
      _DailyGoalPanel.gameObject.SetActive(true);
      _ConnectContainer.SetActive(true);
      UpdateProgBar(ChestRewardManager.Instance.GetCurrentRequirementPoints(), ChestRewardManager.Instance.GetNextRequirementPoints());
      rewardSequence.Play();
    }

    _ConnectButton.Text = Localization.Get(LocalizationKeys.kLabelConnect);
    UIManager.Instance.BackgroundColorController.SetBackgroundColor(Cozmo.UI.BackgroundColorController.BackgroundColor.Bone);
  }

  private void OnDestroy() {
    if (_TimelineSequence != null) {
      _TimelineSequence.Kill();
    }
    if (_CurrentSequence != null) {
      _CurrentSequence.Kill();
    }
    if (_ConnectionFlowInstance != null) {
      GameObject.Destroy(_ConnectionFlowInstance.gameObject);
    }
  }

  private void UpdateProgBar(int currPoints, int reqPoints) {
    float prog = ((float)currPoints / (float)reqPoints);
    _GoalProgressBar.SetProgress(prog, true);
    _GoalProgressText.text = string.Format("{0}", currPoints);
  }

  #region EnvelopeContainer

  [SerializeField]
  private float _EnvelopeShrinkDuration = 0.25f;
  [SerializeField]
  private float _EnvelopeOpenDuration = 0.10f;
  [SerializeField]
  private float _EnvelopeOpenStartScale = 0.9f;
  [SerializeField]
  private float _EnvelopeOpenMaxScale = 1.1f;

  // On tap play animation (crunch down slightly, then pop up and switch to open sprite)
  // On open, release rewards and transition to TimelineReviewContainer.
  private void HandleEnvelopeButton() {
    Transform envelope = _EnvelopeButton.transform;
    Sequence envelopeSequence = DOTween.Sequence();
    _CurrentSequence = envelopeSequence;
    envelopeSequence.Join(_TapToOpenText.DOFade(0.0f, _EnvelopeShrinkDuration));
    envelopeSequence.Join(envelope.DOScaleY(_EnvelopeOpenStartScale, _EnvelopeShrinkDuration));
    envelopeSequence.Append(envelope.DOScaleY(_EnvelopeOpenMaxScale, _EnvelopeOpenDuration));
    envelopeSequence.AppendCallback(StartCurrentStreakSequence);
    envelopeSequence.Play();
  }

  #endregion

  #region TimelineReviewContainer

  [SerializeField]
  private float _TimelineCanvasYOffset = -500f;

  [SerializeField]
  private float _TimelineRewardPopDuration = 0.5f;
  [SerializeField]
  private float _TimelineRewardHoldDuration = 0.75f;
  [SerializeField]
  private float _RewardSpreadRadiusMax = 250.0f;
  [SerializeField]
  private float _RewardSpreadRadiusMin = 100.0f;

  private Vector3 GetSpreadPos(Vector3 origPos) {
    Vector3 newPos = new Vector3();
    int coinFlipX = 1;
    int coinFlipY = 1;
    if (Random.Range(0.0f, 1.0f) > 0.5f) {
      coinFlipX = -1;
    }
    if (Random.Range(0.0f, 1.0f) > 0.5f) {
      coinFlipY = -1;
    }
    newPos = new Vector3(origPos.x + (Random.Range(_RewardSpreadRadiusMin, _RewardSpreadRadiusMax) * coinFlipX),
                         origPos.y + (Random.Range(_RewardSpreadRadiusMin, _RewardSpreadRadiusMax) * coinFlipY), 0.0f);
    return newPos;
  }

  [SerializeField]
  private float _TimelineSettleDuration = 1.5f;
  [SerializeField]
  private float _TimelineOutroDuration = 1.5f;

  private bool _QuickConnect = true;

  private int _FinalIndex = 0;

  // Part of rewards includes generating new daily goals and animating the daily goal
  // panel into place
  // Add rewards to profile and  and auto advance.
  private void StartCurrentStreakSequence() {
    _EnvelopeContainer.SetActive(false);
    // DOMove/Rotate them randomly to fan out rewards above the timeline
    // Start the new session as late as possible to allow for unlockIDs to load
    DataPersistence.DataPersistenceManager.Instance.StartNewSession();
    _QuickConnect = false;
    Sequence rewardSequence = DOTween.Sequence();
    _TimelineSequence = rewardSequence;
    // Create the new session here to minimize risk of cheating/weird timing bugs allowing you to harvest
    // streak rewards by quitting out mid animation
    AnimateRewardIcons(rewardSequence);
    rewardSequence.AppendInterval(_TimelineRewardHoldDuration);
    // Fade in the Timeline as we move the current envelope to the respective location, then hide it and replace it with its own StreakEnvelopeCell
    rewardSequence.Append(_TimelineCanvas.transform.DOLocalMoveY(_TimelineCanvasYOffset, _TimelineSettleDuration).From());
    rewardSequence.Join(UIDefaultTransitionSettings.Instance.CreateFadeInTween(_TimelineCanvas, Ease.Unset, _TimelineSettleDuration));
    // Prepare all TimelineCells and get our envelope Target for the main envelope
    PrepareStreakTimeline(rewardSequence);
    rewardSequence.Join(_OpenEnvelope.transform.DOMove(_FinalEnvelopeTarget.position, _TimelineSettleDuration));
    rewardSequence.Join(_EnvelopeShadow.DOFade(0.0f, _TimelineSettleDuration));
    rewardSequence.Play();
    _TimelineSequence = rewardSequence;
    // If not using Timeline Animations, add HandleTimelineAnimEnd as a callback instead of using the Streak Timeline
    if (DataPersistence.DataPersistenceManager.Instance.CurrentStreak <= 1) {
      rewardSequence.AppendCallback(HandleTimelineAnimEnd);
    }
    _TimelineReviewContainer.SetActive(true);
  }

  private void HandleStreakFillEnd() {
    Sequence rewardSequence = DOTween.Sequence();
    _CurrentSequence = rewardSequence;
    // Fill up each Bar Segment and make the Checkmarks Pop/fade in
    // Fade out and slide out the timeline, send rewards to targets (Energy to EnergyBar at top, Hexes/Sparks to Counters at top, DailyGoal Panel into position)
    // Rewards are independent from Containers
    rewardSequence.Append(_TimelineCanvas.transform.DOLocalMoveY(_TimelineCanvasYOffset, _TimelineOutroDuration));
    rewardSequence.Join(UIDefaultTransitionSettings.Instance.CreateFadeOutTween(_TimelineCanvas, Ease.Unset, _TimelineOutroDuration));
    rewardSequence.AppendCallback(HandleTimelineAnimEnd);
    rewardSequence.Play();
  }

  private void AnimateRewardIcons(Sequence rewardSequence) {
    string itemID = "";
    Transform newReward = null;
    Vector3 rewardTarget = Vector3.zero;
    // SpawnDailyGoalRewards here for each of the newly generated goals
    for (int i = 0; i < DataPersistenceManager.Instance.CurrentSession.DailyGoals.Count; i++) {
      newReward = SpawnDailyGoalReward();
      rewardTarget = GetRandomTarget();
      rewardSequence.Join(newReward.DOScale(0.0f, _TimelineRewardPopDuration).From().SetEase(Ease.OutBack));
      rewardSequence.Join(newReward.DOLocalMove(rewardTarget, _TimelineRewardPopDuration).SetEase(Ease.OutBack));
    }

    Dictionary<RewardedActionData, int> rewardsToCreate = RewardedActionManager.Instance.PendingActionRewards;
    foreach (RewardedActionData reward in rewardsToCreate.Keys) {
      itemID = reward.Reward.ItemID;
      newReward = null;
      rewardTarget = GetRandomTarget();
      // should only be Vector3.zero if out of valid targets
      if (rewardTarget == Vector3.zero) {
        break;
      }
      if (itemID == RewardedActionManager.Instance.EnergyID) {
        newReward = SpawnReward(itemID, reward.Reward.Amount);
        // Spawn reward, grab random target
        rewardSequence.Join(newReward.DOScale(0.0f, _TimelineRewardPopDuration).From().SetEase(Ease.OutBack));
        rewardSequence.Join(newReward.DOLocalMove(rewardTarget, _TimelineRewardPopDuration).SetEase(Ease.OutBack));
      }
      else {
        // Instead of doing a count, do a cluster with non exp rewards
        for (int i = 0; i < reward.Reward.Amount; i++) {
          newReward = SpawnReward(itemID, 1);
          Vector3 spreadTarget = GetSpreadPos(rewardTarget);
          // Spawn reward, grab random target
          rewardSequence.Join(newReward.DOScale(0.0f, _TimelineRewardPopDuration).From().SetEase(Ease.OutBack));
          rewardSequence.Join(newReward.DOLocalMove(spreadTarget, _TimelineRewardPopDuration).SetEase(Ease.OutBack));
        }
      }
    }
    RewardedActionManager.Instance.SendPendingRewardsToInventory();
  }

  // Pulls a random transform from the target list, removes it, and
  // returns its localposition
  private Vector3 GetRandomTarget() {
    if (_MultRewardsTransforms.Count <= 0) {
      return Vector3.zero;
    }
    List<Transform> rewardTargets = new List<Transform>();
    rewardTargets.AddRange(_MultRewardsTransforms);
    Transform toRemove = null;
    toRemove = rewardTargets[UnityEngine.Random.Range(0, rewardTargets.Count)];
    Vector3 rewardTarget = Vector3.zero;
    rewardTarget = toRemove.localPosition;
    _MultRewardsTransforms.Remove(toRemove);

    return rewardTarget;
  }

  // Spawn Daily Goal rewards for Goal Cell animations
  private Transform SpawnDailyGoalReward() {
    Transform newReward = UIManager.CreateUIElement(_RewardPrefab.gameObject, _RewardParticleSource).transform;

    Image rewardImage = newReward.GetComponent<Image>();
    rewardImage.overrideSprite = DailyGoalManager.Instance.GetDailyGoalGenConfig().DailyGoalIcon;

    Text countText = newReward.GetComponentInChildren<Text>();
    countText.gameObject.SetActive(false);

    _ActiveNewGoalTransforms.Add(newReward);

    return newReward;
  }

  // Spawn Reward particles for various item IDs
  private Transform SpawnReward(string rewardID, int count) {
    Transform newReward = UIManager.CreateUIElement(_RewardPrefab.gameObject, _RewardParticleSource).transform;
    Sprite rewardIcon = null;
    // Initialize reward particles with appropriate values
    ItemData iData = ItemDataConfig.GetData(rewardID);
    if (iData != null) {
      rewardIcon = iData.Icon;
    }
    if (rewardIcon != null) {
      Image rewardImage = newReward.GetComponent<Image>();
      rewardImage.overrideSprite = rewardIcon;
    }

    Text countText = newReward.GetComponentInChildren<Text>();

    if (count > 1 && rewardID == RewardedActionManager.Instance.EnergyID) {
      countText.text = Localization.GetWithArgs(LocalizationKeys.kLabelPlusCount, count);
    }
    else {
      countText.gameObject.SetActive(false);
    }


    if (rewardID == RewardedActionManager.Instance.CoinID) {
      _ActiveCoinsTransforms.Add(newReward);
    }
    else if (rewardID == RewardedActionManager.Instance.SparkID) {
      _ActiveSparksTransforms.Add(newReward);
    }
    else {
      _ActiveExpTransforms.Add(newReward);
    }

    return newReward;
  }

  /// <summary>
  /// Prepares the timeline cells.
  /// </summary>
  private void PrepareStreakTimeline(Sequence rewardSequence) {
    int lastDay;
    lastDay = DataPersistence.DataPersistenceManager.Instance.CurrentStreak;
    // If no streak, don't fuck around with the timeline shit
    if (lastDay <= 1) {
      return;
    }
    int firstDay = 1;
    // The Last one in relevant list is the transform we should return
    for (int i = 0; i < kMaxStreakCells; i++) {
      // Pair newCells with Progbars are we create them and drop them into their layout groups
      StreakCell newCell = UIManager.CreateUIElement(_DayCellPrefab, _DayCellLayoutGroup.transform).GetComponent<StreakCell>();
      newCell.ProgBar = UIManager.CreateUIElement(_ProgBarPrefab, _ProgBarLayoutGroup.transform).GetComponent<ProgressBar>();
    }
    _DayCellList.AddRange(_DayCellLayoutGroup.GetComponentsInChildren<StreakCell>());
    if (lastDay > kMaxStreakCells) {
      // Offset First Day based on max streak cells to fix off by 1 bug
      firstDay = (lastDay - kMaxStreakCells) + 1;
    }
    int currDay = firstDay;
    bool finalPlaced = false;
    for (int i = 0; i < kMaxStreakCells; i++) {
      _DayCellList[i].InitializeCell(currDay, finalPlaced == false, lastDay == currDay, i);
      currDay++;
      if (finalPlaced == false) {
        if (lastDay < currDay) {
          // Mark it as final Placed when it is our finalDay because
          // we need to hide the envelope for our current streak for the animation
          finalPlaced = true;
          _FinalIndex = i;
        }
        _DayCellList[i].OnAnimationComplete += HandleStreakAnimComplete;
      }
    }

    rewardSequence.AppendCallback(_DayCellList[0].BeginCheckSequence);
  }

  /// <summary>
  /// Handles whenever each Streak Cell completes its animation/Prog bar fill.
  /// Either start the next one, or handle the StreakFillEnd.
  /// </summary>
  private void HandleStreakAnimComplete(int index) {
    if (index >= _FinalIndex) {
      HandleStreakFillEnd();
    }
    else {
      _DayCellList[index + 1].BeginProgFill();
    }
  }

  #endregion

  #region DailyGoalContainer

  [SerializeField]
  private float _ConnectIntroDuration = 1.5f;
  [SerializeField]
  private float _RewardSendoffDuration = 0.5f;
  [SerializeField]
  private float _RewardSendoffVariance = 0.15f;
  [SerializeField]
  private float _RewardCollectZoneDuration = 0.25f;
  [SerializeField]
  private float _RewardCollectZoneVariance = 0.25f;
  [SerializeField]
  private float _GoalTweenDelay = 0.05f;
  [SerializeField]
  private float _GoalFadeDuration = 0.5f;

  private void HandleTimelineAnimEnd() {
    Sequence goalSequence = DOTween.Sequence();
    _TimelineSequence = goalSequence;
    goalSequence.Join(UIDefaultTransitionSettings.Instance.CreateFadeInTween(_DailyGoalPanel, Ease.Unset, _ConnectIntroDuration));
    goalSequence.Join(UIDefaultTransitionSettings.Instance.CreateFadeInTween(_ConnectCanvas, Ease.Unset, _ConnectIntroDuration));
    _DailyGoalPanel.gameObject.SetActive(true);
    MoveRewardsToCollectZone(goalSequence);
    _ConnectContainer.SetActive(true);
    goalSequence.InsertCallback(_GoalTweenDelay, HandleDailyGoalTweens);
    goalSequence.AppendCallback(() => {
      for (int i = 0; i < _ActiveNewGoalTransforms.Count; i++) {
        _ActiveNewGoalTransforms[i].gameObject.SetActive(false);
      }
      _TimelineReviewContainer.SetActive(false);
    });
    goalSequence.Play();
  }

  private void MoveRewardsToCollectZone(Sequence goalSequence) {
    // Tween Goals to Panel and other rewards to collect zone.
    goalSequence = TweenAllToCollectZone(goalSequence, _ActiveExpTransforms, _GoalsCollectTarget);
    goalSequence = TweenAllToCollectZone(goalSequence, _ActiveCoinsTransforms, _GoalsCollectTarget);
    goalSequence = TweenAllToCollectZone(goalSequence, _ActiveSparksTransforms, _GoalsCollectTarget);
  }

  private Sequence TweenAllToCollectZone(Sequence currSequence, List<Transform> activeTransforms, Transform collectTarget) {
    Vector3 target = collectTarget.position;
    for (int i = 0; i < activeTransforms.Count; i++) {
      Transform currTransform = activeTransforms[i];
      float rewardStaggerValue = UnityEngine.Random.Range(0, _RewardCollectZoneVariance);
      target = GetSpreadPos(collectTarget.localPosition);
      currSequence.Insert(rewardStaggerValue, currTransform.DOLocalMove(target, _RewardCollectZoneDuration).SetEase(Ease.InOutBack));
    }
    return currSequence;
  }

  private void AnimateRewardsToTarget(Sequence goalSequence) {
    goalSequence = TweenAllToFinalTarget(goalSequence, _ActiveExpTransforms, _FinalExpTarget);
    goalSequence = TweenAllToFinalTarget(goalSequence, _ActiveCoinsTransforms, _FinalCoinTarget);
    goalSequence = TweenAllToFinalTarget(goalSequence, _ActiveSparksTransforms, _FinalSparkTarget);
  }

  // Handled as a callback in order for us to have the appropriate delay after DailyGoalPanel becomes
  // active for Start to fire.
  private void HandleDailyGoalTweens() {
    _CurrentSequence = DOTween.Sequence();
    for (int i = 0; i < _ActiveNewGoalTransforms.Count; i++) {
      Transform currTransform = _ActiveNewGoalTransforms[i];
      float stagger = UnityEngine.Random.Range(0, _RewardSendoffVariance);
      // Since Goal reward particles are made based on daily goal count, these lists should always be the same size and correspond
      // 1 to 1
      Transform finalTarget = DailyGoalManager.Instance.GoalPanelInstance.GetGoalSource(DataPersistenceManager.Instance.CurrentSession.DailyGoals[i]);
      _CurrentSequence.Insert(stagger, currTransform.DOMove(finalTarget.position, _RewardSendoffDuration).SetEase(Ease.InBack));
      Image goalIcon = currTransform.GetComponent<Image>();
      if (goalIcon != null) {
        _CurrentSequence.Insert(_RewardSendoffDuration + _RewardSendoffVariance, goalIcon.DOFade(0.0f, _GoalFadeDuration));
      }
    }
    _CurrentSequence.Play();
  }

  private Sequence TweenAllToFinalTarget(Sequence currSequence, List<Transform> activeTransforms, Transform finalTarget) {
    for (int i = 0; i < activeTransforms.Count; i++) {
      Transform currTransform = activeTransforms[i];
      float stagger = UnityEngine.Random.Range(0, _RewardSendoffVariance);
      currSequence.Insert(stagger, currTransform.DOScale(0.0f, _RewardSendoffDuration).SetEase(Ease.InBack));
      currSequence.Insert(stagger, currTransform.DOMove(finalTarget.position, _RewardSendoffDuration).SetEase(Ease.InBack));
    }
    return currSequence;
  }

  private void HandleMockButton() {
    RobotEngineManager.Instance.MockConnect();
    if (DataPersistence.DataPersistenceManager.Instance.IsNewSessionNeeded) {
      DataPersistence.DataPersistenceManager.Instance.StartNewSession();
    }
    if (ConnectionFlowComplete != null) {
      ConnectionFlowComplete();
    }
  }

  private void HandleConnectButton() {
    if (_QuickConnect) {
      StartConnectionFlow();
    }
    else {
      _ConnectButton.Interactable = false;
      Sequence collectSequence = DOTween.Sequence();
      AnimateRewardsToTarget(collectSequence);
      collectSequence.AppendInterval(_RewardSendoffVariance);
      collectSequence.AppendCallback(StartConnectionFlow);
    }
  }

  private void StartConnectionFlow() {
    _ConnectionFlowInstance = GameObject.Instantiate(_ConnectionFlowPrefab.gameObject).GetComponent<ConnectionFlow>();
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
}
