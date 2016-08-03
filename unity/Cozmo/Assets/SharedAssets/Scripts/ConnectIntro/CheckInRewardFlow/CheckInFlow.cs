using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;
using System.Collections.Generic;
using Anki.Cozmo;
using Anki.UI;
using Cozmo;
using Cozmo.UI;

public class CheckInFlow : MonoBehaviour {

  private const int kMaxStreakCells = 7;

  public System.Action ConnectionFlowComplete;
  public System.Action CheckInFlowQuit;

  [SerializeField]
  private Cozmo.UI.CozmoButton _EnvelopeButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _ConnectButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _SimButton;

  [SerializeField]
  private Cozmo.UI.CozmoButton _MockButton;

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
  private Transform _FinalEnvelopeTarget;

  [SerializeField]
  private GameObject _ConnectContainer;
  [SerializeField]
  private CanvasGroup _ConnectCanvas;
  [SerializeField]
  private GameObject _DailyGoalPanel;
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
  private ConnectionFlow _ConnectionFlowPrefab;
  private ConnectionFlow _ConnectionFlowInstance;

  [SerializeField]
  private List<Transform> _MultRewardsTransforms;
  [SerializeField]
  private Transform _DooberSourceTransform;
  [SerializeField]
  private Transform _FinalRewardTarget;
  private List<Transform> _ActiveDooberTransforms = new List<Transform>();

  private Sequence _TimelineSequence;

  private void Awake() {

    _ConnectButton.Initialize(HandleConnectButton, "connect_button", "checkin_dialog");
    _SimButton.Initialize(HandleSimButton, "sim_button", "checkin_dialog");
    _MockButton.Initialize(HandleMockButton, "mock_button", "checkin_dialog");
    _EnvelopeButton.Initialize(HandleEnvelopeButton, "envelope_button", "checkin_dialog");
    //_EnvelopeButton.Text = TODO :: This should set the Envelope Button text to 'TO:PlayerName'
    _EnvelopeContainer.SetActive(false);
    _TimelineReviewContainer.SetActive(false);
    _ConnectContainer.SetActive(false);
    _DailyGoalPanel.SetActive(false);
    // Do Check in Rewards if we need a new session
    if (DataPersistence.DataPersistenceManager.Instance.IsNewSessionNeeded) {
      _EnvelopeContainer.SetActive(true);
      DataPersistence.DataPersistenceManager.Instance.StartNewSession();
    }
    else {
      // Cut ahead to ConnectView otherwise
      _DailyGoalPanel.transform.position = _NewGoalsFinalTarget.position;
      _DailyGoalPanel.SetActive(true);
      _ConnectContainer.SetActive(true);
    }

#if !UNITY_EDITOR
    // hide sim and mock buttons for on device deployments
    _SimButton.gameObject.SetActive(false);
    _MockButton.gameObject.SetActive(false);
#endif

    _ConnectButton.Text = Localization.Get(LocalizationKeys.kLabelConnect);
    UIManager.Instance.BackgroundColorController.SetBackgroundColor(Cozmo.UI.BackgroundColorController.BackgroundColor.Yellow);
  }

  private void OnDestroy() {
    if (_ConnectionFlowInstance != null) {
      GameObject.Destroy(_ConnectionFlowInstance.gameObject);
    }
  }

  #region EnvelopeContainer

  [SerializeField]
  private float _EnvelopeShrinkDuration = 0.25f;
  [SerializeField]
  private float _EnvelopeOpenDuration = 0.10f;
  [SerializeField]
  private float _EnvelopeOpenMinScale = 0.9f;
  [SerializeField]
  private float _EnvelopeOpenMaxScale = 1.1f;

  // On tap play animation (crunch down slightly, then pop up and switch to open sprite)
  // On open, release rewards as doobers and transition to TimelineReviewContainer.
  private void HandleEnvelopeButton() {
    Transform envelope = _EnvelopeButton.transform;
    Sequence envelopeSequence = DOTween.Sequence();
    envelopeSequence.Join(_TapToOpenText.DOFade(0.0f, _EnvelopeShrinkDuration));
    envelopeSequence.Join(envelope.DOScaleY(_EnvelopeOpenMinScale, _EnvelopeShrinkDuration));
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
  private float _TimelineSettleDuration = 1.5f;
  [SerializeField]
  private float _TimelineOutroDuration = 1.5f;
  [SerializeField]
  private float _DailyGoalMiniScale = 0.5f;
  [SerializeField]
  private float _DailyGoalRotation = 45f;

  private int _FinalIndex = 0;

  // Part of rewards includes generating new daily goals and animating the daily goal
  // panel into place
  // Add rewards to profile and  and auto advance.
  private void StartCurrentStreakSequence() {
    _EnvelopeContainer.SetActive(false);
    Sequence rewardSequence = DOTween.Sequence();

    // Create the new session here to minimize risk of cheating/weird timing bugs allowing you to harvest
    // streak rewards by quitting out mid animation
    AnimateRewardIcons(rewardSequence);
    rewardSequence.Join(_DailyGoalPanel.transform.DOMove(_NewGoalsFirstTarget.position, _TimelineRewardPopDuration));
    rewardSequence.Join(_DailyGoalPanel.transform.DOScale(0.0f, _TimelineRewardPopDuration).From());
    rewardSequence.Join(_DailyGoalPanel.transform.DORotate(new Vector3(0f, 0f, _DailyGoalRotation), _TimelineRewardPopDuration));
    rewardSequence.Append(_DailyGoalPanel.transform.DOScale(_DailyGoalMiniScale, _TimelineRewardPopDuration));
    rewardSequence.AppendInterval(_TimelineRewardHoldDuration);
    // Fade in the Timeline as we move the current envelope to the respective location, then hide it and replace it with its own StreakEnvelopeCell
    rewardSequence.Append(_TimelineCanvas.transform.DOMove(new Vector3(0, _TimelineCanvasYOffset), _TimelineSettleDuration).From());
    rewardSequence.Join(_TimelineCanvas.DOFade(0.0f, _TimelineSettleDuration).From());
    rewardSequence.Join(_OpenEnvelope.transform.DOMove(_FinalEnvelopeTarget.position, _TimelineSettleDuration));
    // Prepare all TimelineCells and get our envelope Target for the main envelope
    PrepareStreakTimeline(rewardSequence);
    rewardSequence.Play();
    // DOMove/Rotate them randomly to fan out rewards above the timeline
    _DailyGoalPanel.SetActive(true);
    _TimelineReviewContainer.SetActive(true);
  }

  private void HandleStreakFillEnd() {
    Sequence rewardSequence = DOTween.Sequence();
    // Fill up each Bar Segment and make the Checkmarks Pop/fade in

    // Fade out and slide out the timeline, send rewards to targets (Energy to EnergyBar at top, Hexes/Sparks to Counters at top, DailyGoal Panel into position)
    // Rewards are independent from Containers
    rewardSequence.Append(_TimelineCanvas.transform.DOMove(new Vector3(0, _TimelineCanvasYOffset), _TimelineOutroDuration));
    rewardSequence.Join(_TimelineCanvas.DOFade(0.0f, _TimelineOutroDuration));
    rewardSequence.AppendCallback(HandleTimelineAnimEnd);
    rewardSequence.Play();
  }

  private void AnimateRewardIcons(Sequence dooberSequence) {
    List<Transform> doobTargets = new List<Transform>();
    doobTargets.AddRange(_MultRewardsTransforms);
    Dictionary<RewardedActionData, int> rewardsToCreate = RewardedActionManager.Instance.PendingActionRewards;
    foreach (RewardedActionData reward in rewardsToCreate.Keys) {
      string itemID = reward.Reward.ItemID;
      Transform newDoob = SpawnRewardDoober(itemID, reward.Reward.Amount);
      // Spawn Doober, grab random target
      Transform toRemove = doobTargets[UnityEngine.Random.Range(0, doobTargets.Count)];
      Vector3 doobTarget = toRemove.position;
      doobTargets.Remove(toRemove);
      dooberSequence.Join(newDoob.DOScale(0.0f, _TimelineRewardPopDuration).From().SetEase(Ease.OutBack));
      dooberSequence.Join(newDoob.DOMove(doobTarget, _TimelineRewardPopDuration).SetEase(Ease.OutBack));
      // If all out of available targets, just ignore it.
      if (doobTargets.Count <= 0) {
        break;
      }
    }
    RewardedActionManager.Instance.PendingActionRewards.Clear();
  }

  private Transform SpawnRewardDoober(string rewardID, int count) {
    Transform newDoob = UIManager.CreateUIElement(_RewardPrefab.gameObject, _DooberSourceTransform).transform;
    Sprite rewardIcon = null;
    // Initialize Doober with appropriate values
    ItemData iData = ItemDataConfig.GetData(rewardID);
    if (iData != null) {
      rewardIcon = iData.Icon;
    }

    if (rewardIcon != null) {
      Image doobImage = newDoob.GetComponent<Image>();
      doobImage.overrideSprite = rewardIcon;
      doobImage.SetNativeSize();
    }

    Text countText = newDoob.GetComponentInChildren<Text>();
    if (count > 1) {
      countText.text = Localization.GetWithArgs(LocalizationKeys.kLabelPlusCount, count);
    }
    else {
      countText.gameObject.SetActive(false);
    }

    _ActiveDooberTransforms.Add(newDoob);
    return newDoob;
  }

  /// <summary>
  /// Prepares the timeline cells.
  /// </summary>
  private void PrepareStreakTimeline(Sequence rewardSequence) {
    int lastDay;
    lastDay = DataPersistence.DataPersistenceManager.Instance.CurrentStreak;
    // If no streak, don't fuck around with the timeline shit
    if (lastDay <= 1) {
      HandleTimelineAnimEnd();
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

  private void HandleTimelineAnimEnd() {
    Sequence goalSequence = DOTween.Sequence();
    goalSequence.Join(_DailyGoalPanel.transform.DORotate(Vector3.zero, _ConnectIntroDuration));
    goalSequence.Join(_DailyGoalPanel.transform.DOMove(_NewGoalsFinalTarget.position, _ConnectIntroDuration));
    goalSequence.Join(_DailyGoalPanel.transform.DOScale(1.0f, _ConnectIntroDuration));
    goalSequence.AppendCallback(() => {
      _TimelineReviewContainer.SetActive(false);
      _ConnectContainer.SetActive(true);
    });
    SendDoobersAway(goalSequence);
    goalSequence.Join(_ConnectCanvas.DOFade(0.0f, _ConnectIntroDuration).From());
    goalSequence.Play();
  }

  // BEGONE DOOBERS! OUT OF MY SIGHT! Staggering their start times slightly, send all active doobers to the
  // FinalRewardTarget position
  private void SendDoobersAway(Sequence goalSequence) {
    // If there's more than one reward, give each of them a unique transform to tween out to
    for (int i = 0; i < _ActiveDooberTransforms.Count; i++) {
      Transform currDoob = _ActiveDooberTransforms[i];
      float doobStagger = UnityEngine.Random.Range(0, _RewardSendoffVariance);
      goalSequence.Insert(doobStagger, currDoob.DOScale(0.0f, _RewardSendoffDuration).SetEase(Ease.InBack));
      goalSequence.Insert(doobStagger, currDoob.DOMove(_FinalRewardTarget.position, _RewardSendoffDuration).SetEase(Ease.InBack));
    }
  }

  private void HandleMockButton() {
    RobotEngineManager.Instance.MockConnect();
    if (ConnectionFlowComplete != null) {
      ConnectionFlowComplete();
    }
  }

  private void HandleConnectButton() {
    _ConnectionFlowInstance = GameObject.Instantiate(_ConnectionFlowPrefab.gameObject).GetComponent<ConnectionFlow>();
    _ConnectionFlowInstance.ConnectionFlowComplete += HandleConnectionFlowComplete;
    _ConnectionFlowInstance.ConnectionFlowQuit += HandleConnectionFlowQuit;
    _ConnectionFlowInstance.Play(sim: false);

  }

  private void HandleSimButton() {
    _ConnectionFlowInstance = GameObject.Instantiate(_ConnectionFlowPrefab.gameObject).GetComponent<ConnectionFlow>();
    _ConnectionFlowInstance.ConnectionFlowComplete += HandleConnectionFlowComplete;
    _ConnectionFlowInstance.ConnectionFlowQuit += HandleConnectionFlowQuit;
    _ConnectionFlowInstance.Play(sim: true);
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
    _ConnectionFlowInstance.HandleRobotDisconnect();
  }

  #endregion
}
