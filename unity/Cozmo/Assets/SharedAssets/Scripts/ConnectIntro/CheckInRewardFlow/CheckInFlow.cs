using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;

public class CheckInFlow : MonoBehaviour {

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
  private GameObject _ConnectContainer;
  [SerializeField]
  private CanvasGroup _ConnectCanvas;
  [SerializeField]
  private GameObject _DailyGoalPanel;

  [SerializeField]
  private ConnectionFlow _ConnectionFlowPrefab;
  private ConnectionFlow _ConnectionFlowInstance;

  private Sequence _TimelineSequence;

  private void Start() {

    _ConnectButton.Initialize(HandleConnectButton, "connect_button", "checkin_dialog");
    _SimButton.Initialize(HandleSimButton, "sim_button", "checkin_dialog");
    _MockButton.Initialize(HandleMockButton, "mock_button", "checkin_dialog");
    _EnvelopeButton.Initialize(HandleEnvelopeButton, "envelope_button", "checkin_dialog");
    //_EnvelopeButton.Text = TODO :: This should set the Envelope Button text to 'TO:PlayerName' if we ever have that in player profile
    _EnvelopeContainer.SetActive(false);
    _TimelineReviewContainer.SetActive(false);
    _ConnectContainer.SetActive(false);
    _DailyGoalPanel.SetActive(false);
    // Do Check in Rewards if we need a new session
    if (DataPersistence.DataPersistenceManager.Instance.IsNewSessionNeeded) {
      // TODO : When starting a new session, check for the current streak, then provide adequate rewards as well as generate
      // new goals.
      DataPersistence.DataPersistenceManager.Instance.StartNewSession();
      // TODO : Looks like we can just slap a Daily Goal Panel into the Timeline Container as part of the rewards sequence
      _EnvelopeContainer.SetActive(true);
    }
    else {
      // Cut ahead to ConnectView otherwise
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


  #region EnvelopeContainer

  [SerializeField]
  private float _EnvelopeShrinkDuration = 0.25f;
  [SerializeField]
  private float _EnvelopeOpenDuration = 0.10f;
  [SerializeField]
  private float _EnvelopeOpenMinScale = 0.9f;
  [SerializeField]
  private float _EnvelopeOpenMaxScale = 1.1f;

  // TODO : Show the Envelope, maybe have it slowly bob to draw attention.
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
  private float _TimelineIntroDuration = 1.5f;

  // TODO : Display the current streak, and the current rewards for the day.
  // Part of rewards includes generating new daily goals and animating the daily goal
  // panel into place
  // Add rewards to profile and  and auto advance.
  private void StartCurrentStreakSequence() {
    Sequence rewardSequence = DOTween.Sequence();

    // TODO : Create all Reward Doobers and make Daily Goal Panel Active
    // DOMove/Rotate them randomly to fan out rewards above the timeline
    _DailyGoalPanel.SetActive(true);
    // Fade in the Timeline as we move the current envelope to the respective location, then hide it and replace it with its own StreakEnvelopeCell
    rewardSequence.Join(_TimelineCanvas.transform.DOLocalMove(new Vector3(0, _TimelineCanvasYOffset), _TimelineIntroDuration).From());
    rewardSequence.Join(_TimelineCanvas.DOFade(0.0f, _TimelineIntroDuration).From());
    // Fill up each Bar Segment and make the Checkmarks Pop/fade in - create StreakEnvelopeCell to handle most of this

    // Fade out and slide out the timeline, send rewards to targets (Energy to EnergyBar at top, Hexes/Sparks to Counters at top, DailyGoal Panel into position)
    // Rewards are independent from Containers
    rewardSequence.Append(_TimelineCanvas.transform.DOLocalMove(new Vector3(0, _TimelineCanvasYOffset), _TimelineIntroDuration));
    rewardSequence.Join(_TimelineCanvas.DOFade(0.0f, _TimelineIntroDuration));
    rewardSequence.AppendCallback(HandleCurrentStreakSequenceEnd);
    rewardSequence.Play();
    _EnvelopeContainer.SetActive(false);
    _TimelineReviewContainer.SetActive(true);
  }

  #endregion

  #region DailyGoalContainer

  [SerializeField]
  private float _ConnectIntroDuration = 1.5f;

  private void HandleCurrentStreakSequenceEnd() {
    // TODO: Needs another step between this and starting the current streak where we show the streak bar fill and play relevant logic (Loop for each day in streak)
    Sequence goalSequence = DOTween.Sequence();
    _TimelineReviewContainer.SetActive(false);
    goalSequence.Join(_ConnectCanvas.DOFade(0.0f, _ConnectIntroDuration).From());
    _ConnectContainer.SetActive(true);
    goalSequence.Play();
  }


  // TODO: Display current Daily Goals and Basic Connect Options

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

  #endregion
}
