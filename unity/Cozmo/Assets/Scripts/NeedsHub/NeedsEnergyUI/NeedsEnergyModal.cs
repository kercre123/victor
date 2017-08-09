using Anki.Cozmo;
using Cozmo.Needs;
using Cozmo.Needs.UI;
using Cozmo.UI;
using DG.Tweening;
using UnityEngine;
using Anki.Cozmo.ExternalInterface;

namespace Cozmo.Energy.UI {
  public class NeedsEnergyModal : BaseModal {

    public delegate void OverfeedingShouldTriggerHiccups();
    public event OverfeedingShouldTriggerHiccups CozmoOverfed;

    #region SERIALIZED FIELDS

    [SerializeField]
    private CozmoText[] _InstructionNumberTexts;

    [SerializeField]
    private RectTransform _MetersAnchor;

    [SerializeField]
    private NeedsMetersWidget _MetersWidgetPrefab;

    [SerializeField]
    private CozmoButton _DoneCozmoButton;

    [SerializeField]
    private RepeatableMoveTweenSettings _TweenSettings_CozmoHungryElements = null;

    [SerializeField]
    private RepeatableMoveTweenSettings _TweenSettings_CozmoFullElements = null;

    [SerializeField]
    private float _InactivityTimeOut = 300f; //in seconds

    [SerializeField]
    private CozmoButton _CubeHelpButton;

    [SerializeField]
    private GameObject _CubeHelpGroup;

    [SerializeField]
    private float _AlreadyFullTriggerHiccupOdds = 0.2f;

    #endregion

    #region NON-SERIALIZED FIELDS

    private NeedsMetersWidget _MetersWidget;

    private Sequence _HungryAnimation = null;
    private Sequence _FullAnimation = null;

    private NeedBracketId _LastNeedBracket = NeedBracketId.Count;
    private bool? _WasFull = null;

    //when this timer reaches zero, the modal will close
    //refreshes to _InactivityTimeOut on any touch or feeding
    private float _InactivityTimer = float.MaxValue;

    private BaseModal _CubeHelpModal;

    private bool _WasCozmoOverfed = false;

    #endregion

    #region LIFE SPAN

    public void InitializeEnergyModal() {
      // This avoids calling HubWorldBase.Instance.StopFreeplay();
      // because it doesn't want side effects like turning off the lights
      // Game requests are automatically avoided due to switching to a different activity
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.CancelAllCallbacks();
        robot.CancelAction(RobotActionType.UNKNOWN);
        robot.SetEnableFreeplayLightStates(true);
        robot.OnNumBlocksConnectedChanged += HandleBlockConnectivityChanged;
      }

      _WasCozmoOverfed = false;

      RobotEngineManager.Instance.AddCallback<ReactionTriggerTransition>(HandleRobotReactionaryBehavior);
      RobotEngineManager.Instance.AddCallback<FeedingSFXStageUpdate>(HandleFeedingSFXStageUpdate);
      RobotEngineManager.Instance.AddCallback<BehaviorTransition>(HandleBehaviorTransition);

      NeedsStateManager nsm = NeedsStateManager.Instance;
      nsm.PauseExceptForNeed(NeedId.Energy);

      _MetersWidget = UIManager.CreateUIElement(_MetersWidgetPrefab.gameObject, _MetersAnchor).GetComponent<NeedsMetersWidget>();
      _MetersWidget.Initialize(dasParentDialogName: DASEventDialogName,
                               baseDialog: this,
                               focusOnMeter: NeedId.Energy);

      if (_InstructionNumberTexts != null) {
        for (int i = 0; i < _InstructionNumberTexts.Length; i++) {
          _InstructionNumberTexts[i].SetText(Localization.GetNumber(i + 1));
        }
      }

      if (_DoneCozmoButton != null) {
        _DoneCozmoButton.Initialize(HandleUserClose, "done_button", DASEventDialogName);
      }

      _CubeHelpButton.Initialize(HandleCubeHelpButtonTapped, "cube_help_feed_button", DASEventDialogName);

      _InactivityTimer = _InactivityTimeOut;

      //animate our display energy to the engine energy
      NeedsLevelChangedInternal(NeedsActionId.Feed, false);

      RefreshForCurrentBracket(nsm);

      bool isInFeedOnboarding = OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.FeedIntro);
      if (_OptionalCloseDialogCozmoButton != null && isInFeedOnboarding) {
        _OptionalCloseDialogCozmoButton.gameObject.SetActive(false);
      }

      // if no cubes are connected help them get around it.
      if ((robot != null && robot.LightCubes.Count == 0)) {
        HandleBlockConnectivityChanged(robot.LightCubes.Count);
      }
    }

    protected override void RaiseDialogOpenAnimationFinished() {
      base.RaiseDialogOpenAnimationFinished();
      NeedsStateManager.Instance.OnNeedsActionReceived += HandleLatestNeedsLevelChanged;

      StartFeedingActivity();
    }

    private void Update() {
      if (_InactivityTimer > 0f) {
        _InactivityTimer -= Time.deltaTime;

        if (_InactivityTimer <= 0f && !OnboardingManager.Instance.IsAnyOnboardingActive()) {
          HandleUserClose();
        }
      }
    }

    protected override void RaiseDialogClosed() {
      base.RaiseDialogClosed();

      // Ensure all cube feeding sound FX stop
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Cube_Feeding_Loop_Stop);

      if (_LastNeedBracket != NeedBracketId.Count) {
        AnimateElements(fullElements: _LastNeedBracket == NeedBracketId.Full, hide: true);
      }

      if (_CubeHelpModal != null) {
        _CubeHelpModal.CloseDialog();
      }

      NeedsStateManager.Instance.ResumeAllNeeds();
      NeedsStateManager.Instance.OnNeedsActionReceived -= HandleLatestNeedsLevelChanged;
      RobotEngineManager.Instance.RemoveCallback<FeedingSFXStageUpdate>(HandleFeedingSFXStageUpdate);
      RobotEngineManager.Instance.RemoveCallback<ReactionTriggerTransition>(HandleRobotReactionaryBehavior);
      RobotEngineManager.Instance.RemoveCallback<BehaviorTransition>(HandleBehaviorTransition);


      //RETURN TO FREEPLAY
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.OnNumBlocksConnectedChanged -= HandleBlockConnectivityChanged;
        if (OnboardingManager.Instance.AllowFreeplayOnHubEnter()) {
          robot.SetEnableFreeplayActivity(true);
        }
        else {
          robot.ActivateHighLevelActivity(HighLevelActivity.Selection);
        }
      }
    }

    private void OnApplicationPause(bool pauseStatus) {
      // Since the window closes back they need instructions from start again
      if (pauseStatus && OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.FeedIntro)) {
        OnboardingManager.Instance.RestartPhaseAtStage(0);
      }
      DAS.Debug("NeedsEnergyModal.OnApplicationPause", "Application pause: " + pauseStatus);
      HandleUserClose();
    }

    #endregion

    #region ROBOT CALLBACK HANDLERS

    private void HandleBlockConnectivityChanged(int blocksConnected) {
      bool isFeedCritical = NeedsStateManager.Instance.GetCurrentDisplayValue(NeedId.Energy).Bracket == NeedBracketId.Critical;
      bool isInFeedOnboarding = OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.FeedIntro);
      if ((blocksConnected == 0) && (isFeedCritical || isInFeedOnboarding)) {
        _CubeHelpGroup.SetActive(true);
      }
      else {
        _CubeHelpGroup.SetActive(false);
      }
    }

    private void HandleLatestNeedsLevelChanged(NeedsActionId actionID) {
      NeedsLevelChangedInternal(actionID, true);
    }

    private void NeedsLevelChangedInternal(NeedsActionId actionId, bool triggeredFromMessage) {
      NeedsStateManager nsm = NeedsStateManager.Instance;
      RefreshForCurrentBracket(nsm);

      if (actionId == NeedsActionId.Feed) {
        _InactivityTimer = _InactivityTimeOut;

        _CubeHelpGroup.SetActive(false);
        if (_CubeHelpModal != null) {
          _CubeHelpModal.CloseDialog();
        }

        // If Cozmo was full and the user fed him again, there's a chance he gets the hiccups
        if (triggeredFromMessage &&
          (_WasFull != null) &&
          (_WasFull == true) &&
          !OnboardingManager.Instance.IsOnboardingRequired(OnboardingManager.OnboardingPhases.FeedIntro)) {
          System.Random rand = new System.Random();
          float shouldTriggerFloat = (float)rand.NextDouble();
          if (shouldTriggerFloat < _AlreadyFullTriggerHiccupOdds) {
            // Set the overfed bool here, but don't do anything until
            // the next behavior transition so that the current feeding animation
            // can finish
            _WasCozmoOverfed = true;
          }
        }
      }
    }

    protected override void CleanUp() {
      if (_WasCozmoOverfed) {
        _WasCozmoOverfed = false;
        // Notify Engine
        RobotEngineManager.Instance.Message.NotifyOverfeedingShouldTriggerHiccups = new Anki.Cozmo.ExternalInterface.NotifyOverfeedingShouldTriggerHiccups();
        RobotEngineManager.Instance.SendMessage();
        if (CozmoOverfed != null) {
          CozmoOverfed();
        }
      }
    }

    private void HandleRobotReactionaryBehavior(object messageObject) {
      ReactionTriggerTransition behaviorTransition = messageObject as ReactionTriggerTransition;
      if (behaviorTransition.newTrigger == ReactionTrigger.ReturnedToTreads) {
        var robot = RobotEngineManager.Instance.CurrentRobot;
        if (robot != null) {
          robot.TryResetHeadAndLift(null);
        }
      }
    }

    private void HandleFeedingSFXStageUpdate(FeedingSFXStageUpdate message) {
      uint stageNum = message.stage;
      switch (stageNum) {
      case 0: {
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Cube_Feeding_Loop_Play);
          break;
        }
      case 1: {
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Cube_Feeding_Up);
          break;
        }
      case 2: {
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Cube_Feeding_Down);
          break;
        }
      case 3: {
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Cube_Feeding_Success);
          break;
        }
      case 4: {
          Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Cube_Feeding_Loop_Stop);
          break;
        }
      }
    }

    private void HandleBehaviorTransition(BehaviorTransition message){
      if(_WasCozmoOverfed){
        // Exit feeding so that hiccups can take over
        CloseDialog();
      }
    }


    #endregion

    #region MISC HELPER METHODS

    private void HandleCubeHelpButtonTapped() {
      UIManager.OpenModal(AlertModalLoader.Instance.CubeHelpModalPrefab, new ModalPriorityData(), (newModal) => {
        _CubeHelpModal = newModal;
      });
    }

    private void RefreshForCurrentBracket(NeedsStateManager nsm) {
      NeedBracketId newNeedBracket = nsm.PopLatestEngineValue(NeedId.Energy).Bracket;
      //only hide/show elements if bracket has changed
      if (_LastNeedBracket != newNeedBracket) {
        bool cozmoIsFull = newNeedBracket == NeedBracketId.Full;

        if (_WasFull == null || _WasFull.Value != cozmoIsFull) {
          //first hide unwanted elements, snap hidden if this is initial bracket assignment
          AnimateElements(fullElements: !cozmoIsFull, hide: true, snap: _LastNeedBracket == NeedBracketId.Count);
          //then show wanted elements
          AnimateElements(fullElements: cozmoIsFull, hide: false);
          _WasFull = cozmoIsFull;
        }
        _LastNeedBracket = newNeedBracket;
      }
    }

    private void AnimateElements(bool fullElements = false, bool hide = false, bool snap = false) {
      RepeatableMoveTweenSettings tweenSettings = _TweenSettings_CozmoHungryElements;
      Sequence sequence = _HungryAnimation;
      if (fullElements) {
        tweenSettings = _TweenSettings_CozmoFullElements;
        sequence = _FullAnimation;
      }

      if (sequence != null) {
        sequence.Kill();
      }
      sequence = DOTween.Sequence();

      UIDefaultTransitionSettings settings = UIDefaultTransitionSettings.Instance;
      if (tweenSettings.targets.Length > 0) {
        if (hide ^ snap) {
          settings.ConstructCloseRepeatableMoveTween(ref sequence, tweenSettings);
        }
        else {
          settings.ConstructOpenRepeatableMoveTween(ref sequence, tweenSettings);
        }
      }

      if (snap) {
        sequence.Rewind();
      }
      else {
        sequence.Play();
      }
    }

    private void StartFeedingActivity() {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.ActivateHighLevelActivity(HighLevelActivity.Feeding);
      }
    }

    #endregion

  }
}