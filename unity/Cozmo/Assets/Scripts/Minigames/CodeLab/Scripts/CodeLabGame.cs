using UnityEngine;
using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using Newtonsoft.Json;
using DataPersistence;
using System;
using System.Collections.Generic;
using System.IO;
using Cozmo.UI;
using Cozmo.MinigameWidgets;

namespace CodeLab {
  public class CodeLabSampleProject {
    public uint DisplayOrder;
    public Guid ProjectUUID;
    public uint VersionNum;
    public bool IsVertical;
    public string ProjectIconName;
    public string ProjectXML;
    public string ProjectName;
  }

  public class CodeLabGame : GameBase {

    // When the webview is opened to display the Code Lab workspace,
    // we can display one of the following projects.
    private enum RequestToOpenProjectOnWorkspace {
      DisplayNoProject,     // Unset value
      CreateNewProject,     // Workspace displays only green flag
      DisplayUserProject,   // Display a previously-saved user project. User project UUID saved in _ProjectUUIDToOpen.
      DisplaySampleProject  // Display a previously-saved sample project. Sample project UUID saved in _ProjectUUIDToOpen.
    }

    // Look at CodeLabGameUnlockable.asset in editor to set when unlocked.
    private GameObject _WebViewObject;
    private WebViewObject _WebViewObjectComponent;

    [SerializeField]
    private GameObject _BlackScreenPrefab;

    private RequestToOpenProjectOnWorkspace _RequestToOpenProjectOnWorkspace;
    private string _ProjectUUIDToOpen;
    private List<CodeLabSampleProject> _CodeLabSampleProjects;

    // SessionState tracks the entirety of the current CodeLab Game session, including any currently running program.
    private SessionState _SessionState = new SessionState();

    // Any requests from Scratch that occur whilst Cozmo is resetting to home pose are queued
    private Queue<ScratchRequest> _queuedScratchRequests = new Queue<ScratchRequest>();

    private int _PendingResetToHomeActions = 0;
    private bool _HasQueuedResetToHomePose = false;
    private bool _RequiresResetToNeutralFace = false;

    private uint _ChallengeBookmark = 1;

    private const float kNormalDriveSpeed_mmps = 70.0f;
    private const float kFastDriveSpeed_mmps = 200.0f;
    private const float kDriveDist_mm = 44.0f; // length of one light cube
    private const float kTurnAngle = 90.0f * Mathf.Deg2Rad;
    private const float kToleranceAngle = 10.0f * Mathf.Deg2Rad;
    private const float kLiftSpeed_rps = 2.0f;
    private const float kTimeoutForResetToHomePose_s = 3.0f;

    protected override void InitializeGame(ChallengeConfigBase challengeConfigData) {
      SetRequestToOpenProject(RequestToOpenProjectOnWorkspace.DisplayNoProject, null);

      DAS.Debug("Loading Webview", "");
      UIManager.Instance.ShowTouchCatcher();

      // Since webview takes awhile to load, keep showing the "cozmo is getting ready"
      // load screen instead of the white background that usually shows in front ( shown in gamebase before calling this function )
      SharedMinigameView.HideMiddleBackground();
      SharedMinigameView.HideQuitButton();

      // In GameBase.cs, in the function InitializeChallenge(), the lights are set to allow the Challenge to control them.
      // We don't want this behavior for CodeLab.
      CurrentRobot.SetEnableFreeplayLightStates(true);

      // Load sample projects json from file and cache in list
      string pathToFile = "/Scratch/sample-projects.json";
#if UNITY_EDITOR || UNITY_IOS
      string path = Application.streamingAssetsPath + pathToFile;
#elif UNITY_ANDROID
string path = PlatformUtil.GetResourcesBaseFolder() + pathToFile;
#endif

      string json = File.ReadAllText(path);
      _CodeLabSampleProjects = JsonConvert.DeserializeObject<List<CodeLabSampleProject>>(json);

      LoadWebView();
    }

    protected override void CleanUpOnDestroy() {
      _WebViewObjectComponent = null;

      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.CancelAction(RobotActionType.UNKNOWN);  // Cancel all current actions
        robot.PopDrivingAnimations();
        _SessionState.EndSession();
        robot.ExitSDKMode(false);
        robot.SetVisionMode(Anki.Cozmo.VisionMode.EstimatingFacialExpression, false);
      }
      if (_WebViewObject != null) {
        GameObject.Destroy(_WebViewObject);
        _WebViewObject = null;
      }
      UIManager.Instance.HideTouchCatcher();
    }

    protected override void Update() {
      base.Update();
      // Error case exiting conditions:
      // WebView visibility is the same as being "loaded." Attaching is platform depended so for this edge case
      // avoid cases where the load callback might be called after the quit wait until loaded.
      if (_WebViewObjectComponent != null && _WebViewObjectComponent.GetVisibility() &&
          RobotEngineManager.Instance.CurrentRobot != null && _EndStateIndex != ENDSTATE_QUIT) {
        // Because the SDK agressively turns off every reaction behavior in engine, we can't listen for "placedOnCharger" reaction at the Challenge level.
        // Since all we care about is being on the charger just get that from the robot state and send a single quit request.
        if ((RobotEngineManager.Instance.CurrentRobot.RobotStatus & RobotStatusFlag.IS_ON_CHARGER) != 0) {
          RaiseChallengeQuit();
        }
      }
    }

    private void LoadWebView() {
      // Send EnterSDKMode to engine as we enter this view
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        _SessionState.StartSession();
        robot.PushDrivingAnimations(AnimationTrigger.Count, AnimationTrigger.Count, AnimationTrigger.Count);
        robot.EnterSDKMode(false);
        robot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CodeLabEnter);
      }
      else {
        DAS.Error("Codelab.LoadWebView.NullRobot", "");
      }

      if (_WebViewObject == null) {
        _WebViewObject = new GameObject("WebView", typeof(WebViewObject));
        _WebViewObjectComponent = _WebViewObject.GetComponent<WebViewObject>();
        _WebViewObjectComponent.Init(WebViewCallback, false, @"Mozilla/5.0 (iPhone; CPU iPhone OS 7_1_2 like Mac OS X) AppleWebKit/537.51.2 (KHTML, like Gecko) Version/7.0 Mobile/11D257 Safari/9537.53", WebViewError, WebViewLoaded, true);

        int timesPlayedCodeLab = 0;
        DataPersistenceManager.Instance.Data.DefaultProfile.TotalGamesPlayed.TryGetValue(ChallengeID, out timesPlayedCodeLab);
        if (timesPlayedCodeLab <= 0) {
          LoadURL("extra/tutorial.html");
        }
        else {
          LoadURL("extra/projects.html");
        }
      }
    }

    // SetHeadAngleLazy only calls SetHeadAngle if difference is far enough
    private bool SetHeadAngleLazy(float desiredHeadAngle, RobotCallback callback,
                    QueueActionPosition queueActionPosition = QueueActionPosition.NOW) {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      var currentHeadAngle = robot.HeadAngle;
      var angleDiff = desiredHeadAngle - currentHeadAngle;
      var kAngleEpsilon = 0.05; // ~2.8deg // if closer than this then skip the action

      if (System.Math.Abs(angleDiff) > kAngleEpsilon) {
        robot.SetHeadAngle(desiredHeadAngle, callback, queueActionPosition, useExactAngle: true);
        return true;
      }
      else {
        return false;
      }
    }

    // SetLiftHeightLazy only calls SetLiftHeight if difference is far enough
    private bool SetLiftHeightLazy(float desiredHeightFactor, RobotCallback callback,
                     QueueActionPosition queueActionPosition = QueueActionPosition.NOW,
                     float speed_radPerSec = kLiftSpeed_rps) {
      var robot = RobotEngineManager.Instance.CurrentRobot;
      var currentLiftHeightFactor = robot.LiftHeightFactor; // 0..1
      var heightFactorDiff = desiredHeightFactor - currentLiftHeightFactor;
      var kHeightFactorEpsilon = 0.05; // 5% of range // if closer than this then skip the action

      if (System.Math.Abs(heightFactorDiff) > kHeightFactorEpsilon) {
        robot.SetLiftHeight(desiredHeightFactor, callback, queueActionPosition, speed_radPerSec: speed_radPerSec);
        return true;
      }
      else {
        return false;
      }
    }

    private bool IsResettingToHomePose() {
      return _HasQueuedResetToHomePose || (_PendingResetToHomeActions > 0);
    }

    private void OnTimeoutForResetToHomeCompleted(bool success) {
      if (success && _HasQueuedResetToHomePose) {
        DAS.Info("CodeLab.OnTimeoutForResetToHomeCompleted.DoStuff", "");
        _HasQueuedResetToHomePose = false;
        ResetRobotToHomePos();
      }
      else {
        DAS.Warn("CodeLab.OnTimeoutForResetToHomeCompleted.WasCancelled", "success = " + success + ", wasQueued = " + _HasQueuedResetToHomePose);
      }
    }

    private void QueueResetRobotToHomePos() {
      if (_HasQueuedResetToHomePose) {
        DAS.Warn("CodeLab.QueueResetRobotToHomePos.AlreadyQueued", "Ignoring");
        return;
      }
      var robot = RobotEngineManager.Instance.CurrentRobot;
      robot.CancelAction(RobotActionType.UNKNOWN);  // Cancel any pending actions
      robot.WaitAction(kTimeoutForResetToHomePose_s, this.OnTimeoutForResetToHomeCompleted);
      _HasQueuedResetToHomePose = true;
    }

    private void CancelQueueResetRobotToHomePos() {
      if (_HasQueuedResetToHomePose) {
        _HasQueuedResetToHomePose = false;
        var robot = RobotEngineManager.Instance.CurrentRobot;
        robot.CancelCallback(OnTimeoutForResetToHomeCompleted);
        // Cancelling all wait actions will cause the handler to receive success=false on the next engine tick
        robot.CancelAction(RobotActionType.WAIT);
      }
    }

    private void OnResetToHomeCompleted(bool success) {
      --_PendingResetToHomeActions;

      if (_PendingResetToHomeActions <= 0) {
        if (_PendingResetToHomeActions < 0) {
          DAS.Error("CodeLab.OnResetToHomeCompleted.NegativeHomeActions", "_PendingResetToHomeActions = " + _PendingResetToHomeActions);
        }

        if (_queuedScratchRequests.Count > 0) {
          // Unqueue all requests (could be one per green-flag in the case of parallel scripts)
          while (_queuedScratchRequests.Count > 0) {
            var scratchRequest = _queuedScratchRequests.Dequeue();
            DAS.Info("CodeLab.OnResetToHomeCompleted.AllDone.UnQueue", "command = '" + scratchRequest.command + "', id = " + scratchRequest.requestId);
            HandleBlockScratchRequest(scratchRequest);
          }
        }
        else {
          DAS.Info("CodeLab.OnResetToHomeCompleted.AllDone.NoQueue", "");
        }
      }
    }

    private void ResetRobotToHomePos() {
      // Reset the robot to a default pose (head straight, lift down)
      // Only does anything if not already close to that pose.
      // Any other in-progress actions will be cancelled by the first queued (NOW) action

      // Cancel any queued up requests and just do it immediately
      CancelQueueResetRobotToHomePos();

      Anki.Cozmo.QueueActionPosition queuePos = Anki.Cozmo.QueueActionPosition.NOW;

      RobotEngineManager.Instance.CurrentRobot.TurnOffAllBackpackBarLED();

      if (SetHeadAngleLazy(0.0f, callback: this.OnResetToHomeCompleted, queueActionPosition: queuePos)) {
        ++_PendingResetToHomeActions;
        // Ensure subsequent reset actions run in parallel with this
        queuePos = Anki.Cozmo.QueueActionPosition.IN_PARALLEL;
      }

      if (SetLiftHeightLazy(0.0f, callback: this.OnResetToHomeCompleted, queueActionPosition: queuePos)) {
        ++_PendingResetToHomeActions;
        // Ensure subsequent reset actions run in parallel with this
        queuePos = Anki.Cozmo.QueueActionPosition.IN_PARALLEL;
      }

      // Only wait for the neutral face animation if already waiting for something, or there's definitely a face animation to clear
      if ((_PendingResetToHomeActions > 0) || _RequiresResetToNeutralFace) {
        _RequiresResetToNeutralFace = false;
        RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.NeutralFace, this.OnResetToHomeCompleted, queuePos);
        ++_PendingResetToHomeActions;
      }

      if (_PendingResetToHomeActions > 0) {
        DAS.Info("CodeLab.ResetRobotToHomePos.Started", _PendingResetToHomeActions + " Pending Actions");
      }
      else {
        DAS.Info("CodeLab.ResetRobotToHomePos.AlreadyHome", "");
      }
    }

    private void OnGreenFlagClicked() {
      DAS.Info("Codelab.OnGreenFlagClicked", "");
      _SessionState.OnGreenFlagClicked();
    }

    private void OnScriptStart() {
      DAS.Info("Codelab.OnScriptStart", "");
      _SessionState.StartProgram();
      _queuedScratchRequests.Clear();
      ResetRobotToHomePos();
    }

    private void OnScriptStopped() {
      _SessionState.EndProgram();
      // Release any remaining in-progress scratch blocks (otherwise any waiting on observation will stay alive)
      InProgressScratchBlockPool.ReleaseAllInUse();
      // We enable the facial expressions when first needed, and disable when scripts end (rather than ref-counting need)
      RobotEngineManager.Instance.CurrentRobot.SetVisionMode(VisionMode.EstimatingFacialExpression, false);
      _queuedScratchRequests.Clear();
      QueueResetRobotToHomePos();
    }

    private void OnStopAll() {
      DAS.Info("Codelab.OnStopAll", "");

      _SessionState.OnStopAll();

      // Release any remaining in-progress scratch blocks (otherwise any waiting on observation will stay alive)
      InProgressScratchBlockPool.ReleaseAllInUse();

      _queuedScratchRequests.Clear();

      // Cancel any in-progress actions (unless we're resetting to position)
      // (if we're resetting to position then we will have already canceled other actions,
      // and we don't want to cancel the in-progress reset animations).
      if (!IsResettingToHomePose()) {
        RobotEngineManager.Instance.CurrentRobot.CancelAction(RobotActionType.UNKNOWN);
      }
    }

    private void OnGetCozmoUserAndSampleProjectLists(ScratchRequest scratchRequest) {
      DAS.Info("Codelab.OnGetCozmoUserAndSampleProjectLists", "");

      PlayerProfile defaultProfile = DataPersistenceManager.Instance.Data.DefaultProfile;

      // Provide save and load UI with JSON arrays of the user and sample projects.
      string jsCallback = scratchRequest.argString;

      // Provide JavaScript with a list of user projects as JSON, sorted projects by most recently modified.
      // Don't include XML as the JavaScript chokes on it and it isn't necessary to send along with this list.
      defaultProfile.CodeLabProjects.Sort((proj1, proj2) => -proj1.DateTimeLastModifiedUTC.CompareTo(proj2.DateTimeLastModifiedUTC));
      List<CodeLabProject> copyCodeLabProjectList = new List<CodeLabProject>();
      for (int i = 0; i < defaultProfile.CodeLabProjects.Count; i++) {
        CodeLabProject proj = new CodeLabProject();
        proj.ProjectUUID = defaultProfile.CodeLabProjects[i].ProjectUUID;
        proj.ProjectName = defaultProfile.CodeLabProjects[i].ProjectName;
        copyCodeLabProjectList.Add(proj);
      }
      string userProjectsAsJSON = JsonConvert.SerializeObject(copyCodeLabProjectList);

      // Provide JavaScript with a list of sample projects as JSON, sorted by DisplayOrder.
      // Again, don't include the XML as the JavaScript chokes on it and it isn't necessary to send along with this list.
      _CodeLabSampleProjects.Sort((proj1, proj2) => proj1.DisplayOrder.CompareTo(proj2.DisplayOrder));
      List<CodeLabSampleProject> copyCodeLabSampleProjectList = new List<CodeLabSampleProject>();
      for (int i = 0; i < _CodeLabSampleProjects.Count; i++) {
        CodeLabSampleProject proj = new CodeLabSampleProject();
        proj.ProjectUUID = _CodeLabSampleProjects[i].ProjectUUID;
        proj.ProjectIconName = _CodeLabSampleProjects[i].ProjectIconName;
        proj.ProjectName = _CodeLabSampleProjects[i].ProjectName;
        copyCodeLabSampleProjectList.Add(proj);
      }

      string sampleProjectsAsJSON = JsonConvert.SerializeObject(copyCodeLabSampleProjectList);

      // Call jsCallback with list of serialized Code Lab user projects and sample projects
      _WebViewObjectComponent.EvaluateJS(jsCallback + "('" + userProjectsAsJSON + "','" + sampleProjectsAsJSON + "');");
    }

    private void OnSetChallengeBookmark(ScratchRequest scratchRequest) {
      DAS.Info("Codelab.OnSetChallengeBookmark", "");
      _SessionState.OnChallengesSetSlideNumber(scratchRequest.argUInt);
      _ChallengeBookmark = scratchRequest.argUInt;
    }

    private void OnGetChallengeBookmark(ScratchRequest scratchRequest) {
      DAS.Info("Codelab.OnGetChallengeBookmark", "");

      // Provide js with current challenge bookmark location
      string jsCallback = scratchRequest.argString;

      _WebViewObjectComponent.EvaluateJS(jsCallback + "('" + _ChallengeBookmark + "');");
    }

    private void OnCozmoSaveUserProject(ScratchRequest scratchRequest) {
      DAS.Info("Codelab.OnCozmoSaveUserProject", "UUID=" + scratchRequest.argUUID);
      // Save both new and existing user projects.
      // Check if this is a new project.
      string projectUUID = scratchRequest.argUUID;
      string projectXML = scratchRequest.argString;

      PlayerProfile defaultProfile = DataPersistenceManager.Instance.Data.DefaultProfile;

      if (String.IsNullOrEmpty(projectUUID)) {
        string newUserProjectName = null;
        CodeLabProject newProject = null;
        try {
          if (defaultProfile == null) {
            DAS.Error("OnCozmoSaveUserProject.NullDefaultProfile", "In saving new Code Lab user project, defaultProfile is null");
          }

          // Create new project with the XML stored in projectXML.

          // Create project name: "My Project 1", "My Project 2", etc.
          newUserProjectName = Localization.GetWithArgs(LocalizationKeys.kCodeLabHorizontalUserProjectMyProject, defaultProfile.CodeLabUserProjectNum);
          defaultProfile.CodeLabUserProjectNum++;

          newProject = new CodeLabProject(newUserProjectName, projectXML);

          if (defaultProfile.CodeLabProjects == null) {
            DAS.Error("OnCozmoSaveUserProject.NullCodeLabProjects", "defaultProfile.CodeLabProjects is null");
          }
          defaultProfile.CodeLabProjects.Add(newProject);

          // Inform workspace that the current work on workspace has been saved to a project.
          _WebViewObjectComponent.EvaluateJS("window.newProjectCreated('" + newProject.ProjectUUID + "','" + newProject.ProjectName + "'); ");

          _SessionState.OnCreatedProject(newProject);
        }
        catch (NullReferenceException) {
          DAS.Error("OnCozmoSaveUserProject.NullReferenceExceptionSaveNewProject", "Save new Code Lab user project. CodeLabUserProjectNum = " + defaultProfile.CodeLabUserProjectNum + ", newProject = " + newProject);
        }
      }
      else {
        CodeLabProject projectToUpdate = null;
        try {
          // Project already has a guid. Locate the project then update it.
          projectToUpdate = FindUserProjectWithUUID(projectUUID);
          projectToUpdate.ProjectXML = projectXML;
          projectToUpdate.DateTimeLastModifiedUTC = DateTime.UtcNow;

          _SessionState.OnUpdatedProject(projectToUpdate);
        }
        catch (NullReferenceException) {
          DAS.Error("OnCozmoSaveUserProject.NullReferenceExceptionUpdateProject", "Save existing CodeLab user project. projectUUID = " + projectUUID + ", projectToUpdate = " + projectToUpdate);
        }
      }

      _WebViewObjectComponent.EvaluateJS(@"window.saveProjectCompleted();");
    }

    private void OnCozmoDeleteUserProject(ScratchRequest scratchRequest) {
      DAS.Info("Codelab.OnCozmoDeleteUserProject", "UUID=" + scratchRequest.argString);
      CodeLabProject projectToDelete = FindUserProjectWithUUID(scratchRequest.argString);
      if (projectToDelete != null) {
        PlayerProfile defaultProfile = DataPersistenceManager.Instance.Data.DefaultProfile;
        defaultProfile.CodeLabProjects.Remove(projectToDelete);

        SessionState.DAS_Event("robot.code_lab.deleted_user_project", defaultProfile.CodeLabProjects.Count.ToString());
      }
    }

    private void OnCozmoLoadProjectPage() {
      DAS.Info("Codelab.OnCozmoLoadProjectPage", "");
      SessionState.DAS_Event("robot.code_lab.load_project_page", "");
      LoadURL("extra/projects.html");
    }

    private bool HandleNonBlockScratchRequest(ScratchRequest scratchRequest) {
      // Handle any Scratch requests that don't need an InProgressScratchBlock initializing
      switch (scratchRequest.command) {
      case "cozmoScriptStarted":
        OnScriptStart();
        return true;
      case "cozmoScriptStopped":
        OnScriptStopped();
        return true;
      case "cozmoGreenFlag":
        OnGreenFlagClicked();
        return true;
      case "cozmoStopAll":
        OnStopAll();
        return true;
      case "getCozmoUserAndSampleProjectLists":
        OnGetCozmoUserAndSampleProjectLists(scratchRequest);
        return true;
      case "cozmoSetChallengeBookmark":
        OnSetChallengeBookmark(scratchRequest);
        return true;
      case "cozmoGetChallengeBookmark":
        OnGetChallengeBookmark(scratchRequest);
        return true;
      case "cozmoSaveUserProject":
        OnCozmoSaveUserProject(scratchRequest);
        return true;
      case "cozmoRequestToOpenUserProject":
        SessionState.DAS_Event("robot.code_lab.open_user_project", "");
        OpenCodeLabProject(RequestToOpenProjectOnWorkspace.DisplayUserProject, scratchRequest.argString);
        return true;
      case "cozmoRequestToOpenSampleProject":
        SessionState.DAS_Event("robot.code_lab.open_sample_project", scratchRequest.argString);
        OpenCodeLabProject(RequestToOpenProjectOnWorkspace.DisplaySampleProject, scratchRequest.argString);
        return true;
      case "cozmoRequestToCreateProject":
        SessionState.DAS_Event("robot.code_lab.create_project", "");
        OpenCodeLabProject(RequestToOpenProjectOnWorkspace.CreateNewProject, null);
        return true;
      case "cozmoDeleteUserProject":
        OnCozmoDeleteUserProject(scratchRequest);
        return true;
      case "cozmoLoadProjectPage":
        OnCozmoLoadProjectPage();
        return true;
      case "cozmoCloseCodeLab":
        DAS.Info("Codelab.cozmoCloseCodeLab", "");
        RaiseChallengeQuit();
        return true;
      case "cozmoChallengesOpen":
        _SessionState.OnChallengesOpen();
        return true;
      case "cozmoChallengesClose":
        _SessionState.OnChallengesClose();
        return true;
      default:
        return false;
      }
    }

    private void WebViewCallback(string text) {
      string jsonStringFromJS = string.Format("{0}", text);
      jsonStringFromJS = WWW.UnEscapeURL(jsonStringFromJS);

      Debug.Log("JSON from JavaScript: " + jsonStringFromJS);
      ScratchRequest scratchRequest = JsonConvert.DeserializeObject<ScratchRequest>(jsonStringFromJS, GlobalSerializerSettings.JsonSettings);

      if (HandleNonBlockScratchRequest(scratchRequest)) {
        return;
      }

      if (IsResettingToHomePose()) {
        // Any requests from Scratch that occur whilst Cozmo is resetting to home pose are queued
        DAS.Info("CodeLab.QueueScratchReq", "command = '" + scratchRequest.command + "', id = " + scratchRequest.requestId + ", argString = " + scratchRequest.argString);
        _queuedScratchRequests.Enqueue(scratchRequest);
      }
      else {
        HandleBlockScratchRequest(scratchRequest);
      }
    }

    private void TurnInPlace(float turnAngle, RobotCallback callback) {
      float finalTurnAngle = turnAngle;
      const float kExtraAngle = 2.0f * Mathf.Deg2Rad; // All turns seem to, on average, be this short
      if (finalTurnAngle < 0.0f) {
        finalTurnAngle -= kExtraAngle;
      }
      else {
        finalTurnAngle += kExtraAngle;
      }

      var robot = RobotEngineManager.Instance.CurrentRobot;
      //DAS.Info("CodeLab.TurnInPlace.Start", "Turn " + (finalTurnAngle * Mathf.Rad2Deg) + "d from " + (robot.PoseAngle * Mathf.Rad2Deg) + "d");
      robot.TurnInPlace(finalTurnAngle, 0.0f, 0.0f, kToleranceAngle, callback);
    }

    private void HandleBlockScratchRequest(ScratchRequest scratchRequest) {
      InProgressScratchBlock inProgressScratchBlock = InProgressScratchBlockPool.GetInProgressScratchBlock();
      inProgressScratchBlock.Init(scratchRequest.requestId, _WebViewObjectComponent);

      if (scratchRequest.command == "cozmoDriveForward") {
        // argFloat represents the number selected from the dropdown under the "drive forward" block
        float dist_mm = kDriveDist_mm * scratchRequest.argFloat;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(dist_mm.ToString()));
        RobotEngineManager.Instance.CurrentRobot.DriveStraightAction(kNormalDriveSpeed_mmps, dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock);
      }
      else if (scratchRequest.command == "cozmoDriveForwardFast") {
        // argFloat represents the number selected from the dropdown under the "drive forward fast" block
        float dist_mm = kDriveDist_mm * scratchRequest.argFloat;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(dist_mm.ToString()));
        RobotEngineManager.Instance.CurrentRobot.DriveStraightAction(kFastDriveSpeed_mmps, dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock);
      }
      else if (scratchRequest.command == "cozmoDriveBackward") {
        // argFloat represents the number selected from the dropdown under the "drive backward" block
        float dist_mm = kDriveDist_mm * scratchRequest.argFloat;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(dist_mm.ToString()));
        RobotEngineManager.Instance.CurrentRobot.DriveStraightAction(kNormalDriveSpeed_mmps, -dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock);
      }
      else if (scratchRequest.command == "cozmoDriveBackwardFast") {
        // argFloat represents the number selected from the dropdown under the "drive backward fast" block
        float dist_mm = kDriveDist_mm * scratchRequest.argFloat;
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(dist_mm.ToString()));
        RobotEngineManager.Instance.CurrentRobot.DriveStraightAction(kFastDriveSpeed_mmps, -dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock);
      }
      else if (scratchRequest.command == "cozmoPlayAnimation") {
        Anki.Cozmo.AnimationTrigger animationTrigger = GetAnimationTriggerForScratchName(scratchRequest.argString);
        bool wasMystery = (scratchRequest.argUInt != 0);
        _SessionState.ScratchBlockEvent(scratchRequest.command + (wasMystery ? "Mystery" : ""), DASUtil.FormatExtraData(scratchRequest.argString));
        RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(animationTrigger, inProgressScratchBlock.NeutralFaceThenAdvanceToNextBlock);
        _RequiresResetToNeutralFace = true;
      }
      else if (scratchRequest.command == "cozmoTurnLeft") {
        // Turn 90 degrees to the left
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(kTurnAngle.ToString()));
        TurnInPlace(kTurnAngle, inProgressScratchBlock.CompletedTurn);
      }
      else if (scratchRequest.command == "cozmoTurnRight") {
        // Turn 90 degrees to the right
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData((-kTurnAngle).ToString()));
        TurnInPlace(-kTurnAngle, inProgressScratchBlock.CompletedTurn);
      }
      else if (scratchRequest.command == "cozmoSays") {
        bool hasBadWords = BadWordsFilterManager.Instance.Contains(scratchRequest.argString);
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(hasBadWords.ToString()));  // deliberately don't send string as it's PII
        if (hasBadWords) {
          RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CozmoSaysBadWord, inProgressScratchBlock.AdvanceToNextBlock);
        }
        else {
          RobotEngineManager.Instance.CurrentRobot.SayTextWithEvent(scratchRequest.argString, Anki.Cozmo.AnimationTrigger.Count, callback: inProgressScratchBlock.AdvanceToNextBlock);
        }
      }
      else if (scratchRequest.command == "cozmoHeadAngle") {
        float desiredHeadAngle = (CozmoUtil.kIdealBlockViewHeadValue + CozmoUtil.kIdealFaceViewHeadValue) * 0.5f; // medium setting
        if (scratchRequest.argString == "low") {
          desiredHeadAngle = CozmoUtil.kIdealBlockViewHeadValue;
        }
        else if (scratchRequest.argString == "high") {
          desiredHeadAngle = CozmoUtil.kIdealFaceViewHeadValue;
        }
        // convert angle-factor to an actual angle
        desiredHeadAngle = CozmoUtil.HeadAngleFactorToRadians(desiredHeadAngle, false);

        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData((desiredHeadAngle * Mathf.Rad2Deg).ToString()));

        if (!SetHeadAngleLazy(desiredHeadAngle, inProgressScratchBlock.AdvanceToNextBlock)) {
          // Trigger a short wait action to ensure that our promise is met after this method exits
          RobotEngineManager.Instance.CurrentRobot.WaitAction(0.01f, inProgressScratchBlock.AdvanceToNextBlock);
        }
      }
      else if (scratchRequest.command == "cozmoDockWithCube") {
        _SessionState.ScratchBlockEvent(scratchRequest.command);
        float desiredHeadAngle = CozmoUtil.HeadAngleFactorToRadians(CozmoUtil.kIdealBlockViewHeadValue, false);
        if (!SetHeadAngleLazy(desiredHeadAngle, inProgressScratchBlock.DockWithCube)) {
          // Trigger a very short wait action first instead to ensure callbacks happen after this method exits
          RobotEngineManager.Instance.CurrentRobot.WaitAction(0.01f, callback: inProgressScratchBlock.DockWithCube);
        }
      }
      else if (scratchRequest.command == "cozmoForklift") {
        float liftHeight = 0.5f; // medium setting
        if (scratchRequest.argString == "low") {
          liftHeight = 0.0f;
        }
        else if (scratchRequest.argString == "high") {
          liftHeight = 1.0f;
        }

        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(liftHeight.ToString()));
        if (!SetLiftHeightLazy(liftHeight, inProgressScratchBlock.AdvanceToNextBlock)) {
          // Trigger a short wait action to ensure that our promise is met after this method exits
          RobotEngineManager.Instance.CurrentRobot.WaitAction(0.01f, inProgressScratchBlock.AdvanceToNextBlock);
        }
      }
      else if (scratchRequest.command == "cozmoSetBackpackColor") {
        _SessionState.ScratchBlockEvent(scratchRequest.command, DASUtil.FormatExtraData(scratchRequest.argString));
        RobotEngineManager.Instance.CurrentRobot.SetAllBackpackBarLED(scratchRequest.argUInt);
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if (scratchRequest.command == "cozmoWaitUntilSeeFace") {
        _SessionState.ScratchBlockEvent(scratchRequest.command);
        RobotEngineManager.Instance.AddCallback<RobotObservedFace>(inProgressScratchBlock.RobotObservedFace);
      }
      else if (scratchRequest.command == "cozmoWaitUntilSeeHappyFace") {
        _SessionState.ScratchBlockEvent(scratchRequest.command);
        RobotEngineManager.Instance.CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.EstimatingFacialExpression, true);
        RobotEngineManager.Instance.AddCallback<RobotObservedFace>(inProgressScratchBlock.RobotObservedHappyFace);
      }
      else if (scratchRequest.command == "cozmoWaitUntilSeeSadFace") {
        _SessionState.ScratchBlockEvent(scratchRequest.command);
        RobotEngineManager.Instance.CurrentRobot.SetVisionMode(Anki.Cozmo.VisionMode.EstimatingFacialExpression, true);
        RobotEngineManager.Instance.AddCallback<RobotObservedFace>(inProgressScratchBlock.RobotObservedSadFace);
      }
      else if (scratchRequest.command == "cozmoWaitUntilSeeCube") {
        _SessionState.ScratchBlockEvent(scratchRequest.command);
        RobotEngineManager.Instance.AddCallback<RobotObservedObject>(inProgressScratchBlock.RobotObservedObject);
      }
      else if (scratchRequest.command == "cozmoWaitForCubeTap") {
        _SessionState.ScratchBlockEvent(scratchRequest.command);
        LightCube.TappedAction += inProgressScratchBlock.CubeTapped;
      }
      else {
        inProgressScratchBlock.ReleaseFromPool();
        DAS.Error("HandleBlockScratchRequest.UnknownCommand", "Command = '" + scratchRequest.command + "'");
      }

      return;
    }

    private void OpenCodeLabProject(RequestToOpenProjectOnWorkspace request, string projectUUID) {
      DAS.Info("Codelab.OpenCodeLabProject", "request=" + request + ", UUID=" + projectUUID);
      // Cache the request to open project. These vars will be used after the webview is loaded but before it is visible.
      SetRequestToOpenProject(request, projectUUID);
      ShowGettingReadyScreen();
      LoadURL("index.html");
    }

    // Display blue "Cozmo is getting ready to play" while the Scratch workspace is finishing setup
    private void ShowGettingReadyScreen() {
      _WebViewObjectComponent.SetVisibility(false);

      SharedMinigameView.HideQuitButton();
      PopulateTitleWidget();

      SharedMinigameView.InitializeColor(UIColorPalette.GameBackgroundColor);

      SharedMinigameView.ShowWideSlideWithText(LocalizationKeys.kMinigameLabelCozmoPrep, null);
      SharedMinigameView.ShowShelf();
      SharedMinigameView.ShowSpinnerWidget();
    }

    private void LoadURL(string scratchPathToHTML) {
      if (_SessionState.IsProgramRunning()) {
        // Program is currently running and we won't receive OnScriptStopped notification
        // when the active page and Javascript is nuked, so manually force an OnScriptStopped event
        DAS.Info("Codelab.ManualOnScriptStopped", "");
        OnScriptStopped();
      }

#if UNITY_EDITOR
      string indexFile = Application.streamingAssetsPath + "/Scratch/" + scratchPathToHTML;
#else
      string indexFile = "file://" + PlatformUtil.GetResourcesBaseFolder() + "/Scratch/" + scratchPathToHTML;
#endif

      _WebViewObjectComponent.LoadURL(indexFile);
    }

    private void SetRequestToOpenProject(RequestToOpenProjectOnWorkspace request, string projectUUID) {
      _RequestToOpenProjectOnWorkspace = request;
      _ProjectUUIDToOpen = projectUUID;
    }

    private Anki.Cozmo.AnimationTrigger GetAnimationTriggerForScratchName(string scratchAnimationName) {

      switch (scratchAnimationName) {
      case "bored":
        return Anki.Cozmo.AnimationTrigger.CodeLabBored;
      case "cat":
        return Anki.Cozmo.AnimationTrigger.CodeLabCat;
      case "chatty":
        return Anki.Cozmo.AnimationTrigger.CodeLabChatty;
      case "dejected":
        return Anki.Cozmo.AnimationTrigger.CodeLabDejected;
      case "dog":
        return Anki.Cozmo.AnimationTrigger.CodeLabDog;
      case "excited":
        return Anki.Cozmo.AnimationTrigger.CodeLabExcited;
      case "frustrated":
        return Anki.Cozmo.AnimationTrigger.CodeLabFrustrated;
      case "happy":
        return Anki.Cozmo.AnimationTrigger.CodeLabHappy;
      case "sleep":
        return Anki.Cozmo.AnimationTrigger.CodeLabSleep;
      case "sneeze":
        return Anki.Cozmo.AnimationTrigger.CodeLabSneeze;
      case "surprise":
        return Anki.Cozmo.AnimationTrigger.CodeLabSurprise;
      case "thinking":
        return Anki.Cozmo.AnimationTrigger.CodeLabThinking;
      case "unhappy":
        return Anki.Cozmo.AnimationTrigger.CodeLabUnhappy;
      case "victory":
        return Anki.Cozmo.AnimationTrigger.CodeLabVictory;
      default:
        DAS.Error("CodeLab.BadTriggerName", "Unexpected name '" + scratchAnimationName + "'");
        break;
      }
      return Anki.Cozmo.AnimationTrigger.MeetCozmoFirstEnrollmentCelebration;
    }

    CodeLabProject FindUserProjectWithUUID(string uuid) {
      Guid projectGuid = new Guid(uuid);
      CodeLabProject codeLabProject = null;

      Predicate<DataPersistence.CodeLabProject> findProject = (CodeLabProject p) => { return p.ProjectUUID == projectGuid; };
      codeLabProject = DataPersistenceManager.Instance.Data.DefaultProfile.CodeLabProjects.Find(findProject);

      return codeLabProject;
    }

    private void WebViewError(string text) {
      Debug.LogError(string.Format("CallOnError[{0}]", text));
    }

    private void WebViewLoaded(string text) {
      Debug.Log(string.Format("CallOnLoaded[{0}]", text));
      RequestToOpenProjectOnWorkspace cachedRequestToOpenProject = _RequestToOpenProjectOnWorkspace;

      switch (_RequestToOpenProjectOnWorkspace) {
      case RequestToOpenProjectOnWorkspace.CreateNewProject:
        // Open workspace and display only a green flag on the workspace.
        _WebViewObjectComponent.EvaluateJS("window.putStarterGreenFlagOnWorkspace();");
        break;

      case RequestToOpenProjectOnWorkspace.DisplayUserProject:
        if (_ProjectUUIDToOpen != null) {
          CodeLabProject projectToOpen = FindUserProjectWithUUID(_ProjectUUIDToOpen);
          if (projectToOpen != null) {
            // Escape quotes in XML
            // TODO need to do the same for project name and project uuid?
            String projectXMLEscaped = projectToOpen.ProjectXML.Replace("\"", "\\\"");

            // Open requested project in webview
            _WebViewObjectComponent.EvaluateJS("window.openCozmoProject('" + projectToOpen.ProjectUUID + "','" + projectToOpen.ProjectName + "',\"" + projectXMLEscaped + "\",'false');");
          }
        }
        else {
          DAS.Error("CodeLab.NullUserProject", "User project empty for _ProjectUUIDToOpen = '" + _ProjectUUIDToOpen + "'");
        }
        break;

      case RequestToOpenProjectOnWorkspace.DisplaySampleProject:
        if (_ProjectUUIDToOpen != null) {
          Guid projectGuid = new Guid(_ProjectUUIDToOpen);
          CodeLabSampleProject codeLabSampleProject = null;

          Predicate<CodeLabSampleProject> findProject = (CodeLabSampleProject p) => { return p.ProjectUUID == projectGuid; };
          codeLabSampleProject = _CodeLabSampleProjects.Find(findProject);

          // Escape quotes in XML
          // TODO need to do the same for project name and project uuid?
          String projectXMLEscaped = codeLabSampleProject.ProjectXML.Replace("\"", "\\\"");

          // Open requested project in webview
          String sampleProjectName = Localization.Get(codeLabSampleProject.ProjectName);
          _WebViewObjectComponent.EvaluateJS("window.openCozmoProject('" + codeLabSampleProject.ProjectUUID + "','" + sampleProjectName + "',\"" + projectXMLEscaped + "\",'true');");
        }
        else {
          DAS.Error("CodeLab.NullSampleProject", "Sample project empty for _ProjectUUIDToOpen = '" + _ProjectUUIDToOpen + "'");
        }
        break;

      default:
        break;
      }

      SetRequestToOpenProject(RequestToOpenProjectOnWorkspace.DisplayNoProject, null);

      // If we are displaying workspace, give the webview a little time to get ready.
      uint delayInSeconds = 0;
      if (cachedRequestToOpenProject != RequestToOpenProjectOnWorkspace.DisplayNoProject) {
#if UNITY_EDITOR || UNITY_IOS
        delayInSeconds = 1;
#elif UNITY_ANDROID
        delayInSeconds = 2;
#endif
      }
      Invoke("UnhideWebView", delayInSeconds);
    }

    void UnhideWebView() {
      SharedMinigameView.HideMiddleBackground();

      // Add in black screen, so it looks like less of a pop.
      SharedMinigameView.ShowFullScreenGameStateSlide(_BlackScreenPrefab, "BlackScreen");
      SharedMinigameView.HideSpinnerWidget();

      _WebViewObjectComponent.SetVisibility(true);

    }
  }
}
