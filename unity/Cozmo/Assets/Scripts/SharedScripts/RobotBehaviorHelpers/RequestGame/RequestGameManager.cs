using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using Anki.Cozmo.VoiceCommand;
using Cozmo.Challenge;
using Cozmo.UI;
using System;
using System.Diagnostics;

namespace Cozmo.RequestGame {
  public class RequestGameManager {
    private static RequestGameManager _Instance;

    public static RequestGameManager Instance { get { return _Instance; } }

    public static void CreateInstance() {
      _Instance = new RequestGameManager();
    }

    public delegate void RequestGameConfirmedHandler(string challengeId);
    public event RequestGameConfirmedHandler OnRequestGameConfirmed;

    AlertModalData _RequestGameAlertData = null;
    private Stopwatch _Stopwatch = null;
    private string _CurrentChallengeId = null;
    private UnlockId _CurrentChallengeUnlockID = UnlockId.Count;
    private AlertModal _RequestDialog = null;

    private ChallengeDataList ChallengeList {
      get { return ChallengeDataList.Instance; }
    }

    private RequestGameManager() {
      RobotEngineManager.Instance.AddCallback<RequestGameStart>(HandleAskForMinigame);
      RobotEngineManager.Instance.AddCallback<DenyGameStart>(HandleExternalRejection);
      VoiceCommandManager.Instance.OnUserPromptResponse += HandleUserResponseToPrompt;
    }

    public void EnableRequestGameBehaviorGroups() {
      RobotEngineManager.Instance.Message.CanCozmoRequestGame = Singleton<CanCozmoRequestGame>.Instance.Initialize(true);
      RobotEngineManager.Instance.SendMessage();
    }

    public void DisableRequestGameBehaviorGroups() {
      RobotEngineManager.Instance.Message.CanCozmoRequestGame = Singleton<CanCozmoRequestGame>.Instance.Initialize(false);
      RobotEngineManager.Instance.SendMessage();
    }

    private ChallengeData GetDataForGameID(UnlockId id) {
      // Mark always once the popup shows up, after that should show the others
      if (RequestGameListConfig.Instance != null) {
        RequestGameListConfig requestGameConfig = RequestGameListConfig.Instance;
        for (int i = 0; i < requestGameConfig.RequestList.Length; ++i) {
          if (requestGameConfig.RequestList[i].RequestGameID.Value.Equals(id)) {
            return ChallengeList.GetChallengeDataById(requestGameConfig.RequestList[i].ChallengeID);
          }
        }
      }
      return null;
    }

    private void HandleAskForMinigame(Anki.Cozmo.ExternalInterface.RequestGameStart messageObject) {
      ChallengeData data = RequestGameManager.Instance.GetDataForGameID(messageObject.gameRequested);
      // Do not send the minigame message if the challenge is invalid or currently not unlocked.
      if (data == null || !UnlockablesManager.Instance.IsUnlocked(data.UnlockId.Value)) {
        return;
      }

      _RequestGameAlertData = new AlertModalData("request_game_alert",
                LocalizationKeys.kRequestGameTitle,
                LocalizationKeys.kRequestGameDescription,
                new AlertModalButtonData("yes_play_game_button", LocalizationKeys.kButtonYes, HandleMiniGameConfirm),
                new AlertModalButtonData("no_cancel_game_button", LocalizationKeys.kButtonNo, HandleMiniGameRejection),
                icon: data.ChallengeIcon,
                dialogCloseAnimationFinishedCallback: HandleRequestDialogClose,
                titleLocArgs: new object[] { Localization.Get(data.ChallengeTitleLocKey) });

      var requestGamePriority = new ModalPriorityData(ModalPriorityLayer.VeryLow, 2,
                  LowPriorityModalAction.CancelSelf,
                  HighPriorityModalAction.Stack);

      Action<AlertModal> requestGameCreated = (alertModal) => {
        ContextManager.Instance.AppFlash(playChime: true);
        // start response timer
        _Stopwatch = new Stopwatch(); // allways create new sintance - GC can take care of previous ones.
        _Stopwatch.Start();
        _CurrentChallengeId = data.ChallengeID;
        _CurrentChallengeUnlockID = data.UnlockId.Value;

        // Hook up callbacks
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Gp_Shared_Request_Game);
        _RequestDialog = alertModal;
      };

      Action<UIManager.CreationCancelledReason> requestGameCancelled = (cancelReason) => {
        HandleMiniGameRejection();
        HandleRequestDialogClose();
      };

      UIManager.OpenAlert(_RequestGameAlertData, requestGamePriority, requestGameCreated,
            creationCancelledCallback: requestGameCancelled,
            overrideCloseOnTouchOutside: false);
    }

    private void HandleUserResponseToPrompt(bool positiveResponse) {
      if (_RequestGameAlertData != null) {
        if (positiveResponse) {
          HandleMiniGameConfirm();
        }
        else {
          HandleMiniGameRejection();
        }
        if (_RequestDialog != null) {
          _RequestDialog.CloseDialog();
        }
      }
    }

    private void HandleMiniGameRejection() {
      _RequestGameAlertData = null;
      float elapsedSec = 0.0f;
      if (_Stopwatch != null) {
        _Stopwatch.Stop();
        elapsedSec = _Stopwatch.ElapsedMilliseconds / 1000.0f;
      }
      // If current challenge ID is null, robot has already canceled this request, so don't fire redundant DAS events
      if (_CurrentChallengeId != null) {
        DAS.Event("robot.request_app", _CurrentChallengeId, DASUtil.FormatExtraData("fail"));
        DAS.Event("robot.request_app_time", _CurrentChallengeId, DASUtil.FormatExtraData(elapsedSec.ToString()));
        _CurrentChallengeId = null;
      }

      RobotEngineManager.Instance.SendDenyGameStart();
    }

    private void HandleMiniGameConfirm() {
      _RequestGameAlertData = null;
      float elapsedSec = 0.0f;
      if (_Stopwatch != null) {
        _Stopwatch.Stop();
        elapsedSec = _Stopwatch.ElapsedMilliseconds / 1000.0f;
      }
      DAS.Event("robot.request_app", _CurrentChallengeId, DASUtil.FormatExtraData("success"));
      DAS.Event("robot.request_app_time", _CurrentChallengeId, DASUtil.FormatExtraData(elapsedSec.ToString()));

      if (_RequestDialog != null) {
        _RequestDialog.DisableAllButtons();
        _RequestDialog.DialogClosed -= HandleRequestDialogClose;
      }

      if (OnRequestGameConfirmed != null) {
        if (_CurrentChallengeUnlockID != UnlockId.Count) {
          OnRequestGameConfirmed(_CurrentChallengeId);
        }
        else {
          int cubesRequired = ChallengeDataList.Instance.GetChallengeDataById(_CurrentChallengeId).ChallengeConfig.NumCubesRequired();
          if (RobotEngineManager.Instance.CurrentRobot.LightCubes.Count < cubesRequired) {
            // challenge request has become null due to cube(s) disconnecting.
            string challengeTitle = Localization.Get(ChallengeDataList.Instance.GetChallengeDataById(_CurrentChallengeId).ChallengeTitleLocKey);
            NeedCubesAlertHelper.OpenNeedCubesAlert(RobotEngineManager.Instance.CurrentRobot.LightCubes.Count,
                                cubesRequired,
                                challengeTitle,
                                _RequestDialog.PriorityData);
          }
          else {
            DAS.Error("HomeView.HandleMiniGameConfirm", "challenge request is null for an unknown reason");
          }
        }
      }

      _CurrentChallengeId = null;
    }

    private void HandleExternalRejection(object messageObject) {
      DAS.Info(this, "HandleExternalRejection");
      float elapsedSec = 0.0f;
      if (_Stopwatch != null) {
        _Stopwatch.Stop();
        elapsedSec = _Stopwatch.ElapsedMilliseconds / 1000.0f;
      }
      // If current challenge ID is null, we have already responded to this request, so don't fire redundant DAS events
      if (_CurrentChallengeId != null) {
        DAS.Event("robot.request_app", _CurrentChallengeId, DASUtil.FormatExtraData("robot_canceled"));
        DAS.Event("robot.request_app_time", _CurrentChallengeId, DASUtil.FormatExtraData(elapsedSec.ToString()));
        _CurrentChallengeId = null;
      }

      if (_RequestDialog != null) {
        _RequestDialog.CloseDialog();
      }
    }

    private void HandleRequestDialogClose() {
      DAS.Info(this, "HandleUnexpectedClose");
      if (_RequestDialog != null) {
        _RequestDialog.DialogCloseAnimationFinished -= HandleRequestDialogClose;
      }
    }
  }
}