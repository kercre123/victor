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

#if ANKI_DEV_CHEATS
    // Look at CodeLabGameUnlockable.asset in editor to set when unlocked.
    private GameObject _WebViewObject;
    private WebViewObject _WebViewObjectComponent;

    private RequestToOpenProjectOnWorkspace _RequestToOpenProjectOnWorkspace;
    private string _ProjectUUIDToOpen;
    private List<CodeLabSampleProject> _CodeLabSampleProjects;

    private const float kSlowDriveSpeed_mmps = 30.0f;
    private const float kMediumDriveSpeed_mmps = 100.0f;
    private const float kFastDriveSpeed_mmps = 200.0f;
    private const float kDriveDist_mm = 44.0f; // length of one light cube
    private const float kTurnAngle = 90.0f * Mathf.Deg2Rad;
    private const float kToleranceAngle = 10.0f * Mathf.Deg2Rad;

    private const string kUserProjectName = "My Project"; // TODO Move to internationalization files

    protected override void InitializeGame(MinigameConfigBase minigameConfigData) {
      SetRequestToOpenProject(RequestToOpenProjectOnWorkspace.DisplayNoProject, null);

      DAS.Debug("Loading Webview", "");
      UIManager.Instance.ShowTouchCatcher();

      // Since webview takes awhile to load, keep showing the "cozmo is getting ready"
      // load screen instead of the white background that usually shows in front ( shown in gamebase before calling this function )
      SharedMinigameView.HideMiddleBackground();
      SharedMinigameView.HideQuitButton();

      // In GameBase.cs, in the function InitializeMinigame(), the lights are set to allow the minigame to control them.
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

      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.ExitSDKMode(false);
      }
      if (_WebViewObject != null) {
        GameObject.Destroy(_WebViewObject);
        _WebViewObject = null;
      }
      UIManager.Instance.HideTouchCatcher();
    }

    protected override void Update() {
      base.Update();
      // Because the SDK agressively turns off every reaction behavior in engine, we can't listen for "placedOnCharger" reaction at the minigame level.
      // Since all we care about is being on the charger just get that from the robot state and send a single quit request.
      if (_WebViewObject != null && RobotEngineManager.Instance.CurrentRobot != null && _EndStateIndex != ENDSTATE_QUIT) {
        if ((RobotEngineManager.Instance.CurrentRobot.RobotStatus & RobotStatusFlag.IS_ON_CHARGER) != 0) {
          RaiseMiniGameQuit();
        }
      }
    }

    private void LoadWebView() {
      // Send EnterSDKMode to engine as we enter this view
      var robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot != null) {
        robot.EnterSDKMode(false);
        robot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CodeLabEnter);
      }

      if (_WebViewObject == null) {
        _WebViewObject = new GameObject("WebView", typeof(WebViewObject));
        _WebViewObjectComponent = _WebViewObject.GetComponent<WebViewObject>();
        _WebViewObjectComponent.Init(WebViewCallback, false, @"Mozilla/5.0 (iPhone; CPU iPhone OS 7_1_2 like Mac OS X) AppleWebKit/537.51.2 (KHTML, like Gecko) Version/7.0 Mobile/11D257 Safari/9537.53", WebViewError, WebViewLoaded, true);

        // TODO Change to tutorial for first run, otherwise save/load UI.
        LoadURL("extra/projects.html");
      }
    }

    private void WebViewCallback(string text) {
      PlayerProfile defaultProfile = DataPersistenceManager.Instance.Data.DefaultProfile;

      string jsonStringFromJS = string.Format("{0}", text);
      Debug.Log("JSON from JavaScript: " + jsonStringFromJS);
      ScratchRequest scratchRequest = JsonConvert.DeserializeObject<ScratchRequest>(jsonStringFromJS, GlobalSerializerSettings.JsonSettings);

      InProgressScratchBlock inProgressScratchBlock = InProgressScratchBlockPool.GetInProgressScratchBlock();
      inProgressScratchBlock.Init(scratchRequest.requestId, _WebViewObjectComponent);

      if (scratchRequest.command == "getCozmoUserAndSampleProjectLists") {
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
      else if (scratchRequest.command == "cozmoSaveUserProject") {
        // Save both new and existing user projects.
        // Check if this is a new project.
        string projectUUID = scratchRequest.argUUID;
        string projectXML = scratchRequest.argString;

        if (String.IsNullOrEmpty(projectUUID)) {
          // Create new project with the XML stored in projectXML.

          // Create project name: "My Project 1", "My Project 2", etc.
          string newUserProjectName = kUserProjectName + " " + defaultProfile.CodeLabUserProjectNum;
          defaultProfile.CodeLabUserProjectNum++;

          CodeLabProject newProject = new CodeLabProject(newUserProjectName, projectXML);
          defaultProfile.CodeLabProjects.Add(newProject);

          // Inform workspace that the current work on workspace has been saved to a project.
          _WebViewObjectComponent.EvaluateJS("window.newProjectCreated('" + newProject.ProjectUUID + "','" + newProject.ProjectName + "'); ");
        }
        else {
          // Project already has a guid. Locate the project then update it.
          CodeLabProject projectToUpdate = FindUserProjectWithUUID(projectUUID);
          projectToUpdate.ProjectXML = projectXML;
          projectToUpdate.DateTimeLastModifiedUTC = DateTime.UtcNow;
        }
      }
      else if (scratchRequest.command == "cozmoRequestToOpenUserProject") {
        OpenCodeLabProject(RequestToOpenProjectOnWorkspace.DisplayUserProject, scratchRequest.argString);
      }
      else if (scratchRequest.command == "cozmoRequestToOpenSampleProject") {
        OpenCodeLabProject(RequestToOpenProjectOnWorkspace.DisplaySampleProject, scratchRequest.argString);
      }
      else if (scratchRequest.command == "cozmoRequestToCreateProject") {
        OpenCodeLabProject(RequestToOpenProjectOnWorkspace.CreateNewProject, null);
      }
      else if (scratchRequest.command == "cozmoDeleteUserProject") {
        CodeLabProject projectToDelete = FindUserProjectWithUUID(scratchRequest.argString);
        if (projectToDelete != null) {
          defaultProfile.CodeLabProjects.Remove(projectToDelete);
        }
      }
      else if (scratchRequest.command == "cozmoLoadProjectPage") {
        LoadURL("extra/projects.html");
      }
      else if (scratchRequest.command == "cozmoCloseCodeLab") {
        RaiseMiniGameQuit();
      }
      else if (scratchRequest.command == "cozmoDriveForward") {
        // Here, argFloat represents the number selected from the dropdown under the "drive forward" block
        float dist_mm = kDriveDist_mm * scratchRequest.argFloat;
        RobotEngineManager.Instance.CurrentRobot.DriveStraightAction(GetDriveSpeed(scratchRequest), dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock);
      }
      else if (scratchRequest.command == "cozmoDriveBackward") {
        // Here, argFloat represents the number selected from the dropdown under the "drive backward" block
        float dist_mm = kDriveDist_mm * scratchRequest.argFloat;
        RobotEngineManager.Instance.CurrentRobot.DriveStraightAction(-GetDriveSpeed(scratchRequest), -dist_mm, false, inProgressScratchBlock.AdvanceToNextBlock);
      }
      else if (scratchRequest.command == "cozmoPlayAnimation") {
        Anki.Cozmo.AnimationTrigger animationTrigger = GetAnimationTriggerForScratchName(scratchRequest.argString);
        RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(animationTrigger, inProgressScratchBlock.AdvanceToNextBlock);
      }
      else if (scratchRequest.command == "cozmoTurnLeft") {
        // Turn 90 degrees to the left
        RobotEngineManager.Instance.CurrentRobot.TurnInPlace(kTurnAngle, 0.0f, 0.0f, kToleranceAngle, inProgressScratchBlock.AdvanceToNextBlock);
      }
      else if (scratchRequest.command == "cozmoTurnRight") {
        // Turn 90 degrees to the right
        RobotEngineManager.Instance.CurrentRobot.TurnInPlace(-kTurnAngle, 0.0f, 0.0f, kToleranceAngle, inProgressScratchBlock.AdvanceToNextBlock);
      }
      else if (scratchRequest.command == "cozmoSays") {
        bool hasBadWords = BadWordsFilterManager.Instance.Contains(scratchRequest.argString);
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

        if (System.Math.Abs(desiredHeadAngle - RobotEngineManager.Instance.CurrentRobot.HeadAngle) > float.Epsilon) {
          RobotEngineManager.Instance.CurrentRobot.SetHeadAngle(desiredHeadAngle, inProgressScratchBlock.AdvanceToNextBlock);
        }
      }
      else if (scratchRequest.command == "cozmoDockWithCube") {
        RobotEngineManager.Instance.CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue, callback: inProgressScratchBlock.DockWithCube);
      }
      else if (scratchRequest.command == "cozmoForklift") {
        float liftHeight = 0.5f; // medium setting
        if (scratchRequest.argString == "low") {
          liftHeight = 0.0f;
        }
        else if (scratchRequest.argString == "high") {
          liftHeight = 1.0f;
        }

        RobotEngineManager.Instance.CurrentRobot.SetLiftHeight(liftHeight, inProgressScratchBlock.AdvanceToNextBlock, QueueActionPosition.NOW, 2);
      }
      else if (scratchRequest.command == "cozmoSetBackpackColor") {
        RobotEngineManager.Instance.CurrentRobot.SetAllBackpackBarLED(scratchRequest.argUInt);
        inProgressScratchBlock.AdvanceToNextBlock(true);
      }
      else if (scratchRequest.command == "cozmoWaitUntilSeeFace") {
        RobotEngineManager.Instance.AddCallback<RobotObservedFace>(inProgressScratchBlock.RobotObservedFace);
      }
      else if (scratchRequest.command == "cozmoWaitUntilSeeCube") {
        RobotEngineManager.Instance.AddCallback<RobotObservedObject>(inProgressScratchBlock.RobotObservedObject);
      }
      else if (scratchRequest.command == "cozmoWaitForCubeTap") {
        LightCube.TappedAction += inProgressScratchBlock.CubeTapped;
      }
      else {
        Debug.LogError("Scratch: no match for command");
      }

      return;
    }

    private void OpenCodeLabProject(RequestToOpenProjectOnWorkspace request, string projectUUID) {
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

    // Check if cozmoDriveFaster JavaScript bool arg is set by checking ScratchRequest.argBool and set speed appropriately.
    private float GetDriveSpeed(ScratchRequest scratchRequest) {
      float driveSpeed_mmps = kSlowDriveSpeed_mmps;
      if (scratchRequest.argString == "medium") {
        driveSpeed_mmps = kMediumDriveSpeed_mmps;
      }
      else if (scratchRequest.argString == "fast") {
        driveSpeed_mmps = kFastDriveSpeed_mmps;
      }

      return driveSpeed_mmps;
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
          _WebViewObjectComponent.EvaluateJS("window.openCozmoProject('" + codeLabSampleProject.ProjectUUID + "','" + codeLabSampleProject.ProjectName + "',\"" + projectXMLEscaped + "\",'true');");
        }
        else {
          DAS.Error("CodeLab.NullSampleProject", "Sample project empty for _ProjectUUIDToOpen = '" + _ProjectUUIDToOpen + "'");
        }
        break;

      default:
        break;
      }

      SetRequestToOpenProject(RequestToOpenProjectOnWorkspace.DisplayNoProject, null);

      SharedMinigameView.HideMiddleBackground();

      // TODO Need to pause before setting this?
      _WebViewObjectComponent.SetVisibility(true);
    }
#else // ANKI_DEV_CHEATS SECRET
    protected override void InitializeGame(MinigameConfigBase minigameConfigData) {
    }

    protected override void CleanUpOnDestroy() {
      // required function
    }

#endif

  }

}
