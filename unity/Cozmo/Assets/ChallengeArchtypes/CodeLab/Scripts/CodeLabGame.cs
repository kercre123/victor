using UnityEngine;
using Anki.Cozmo.ExternalInterface;
using Newtonsoft.Json;
using System.Collections.Generic;

namespace CodeLab {
  public class CodeLabGame : GameBase {

#if ANKI_DEV_CHEATS
    // Look at CodeLabGameUnlockable.asset in editor to set when unlocked.
    private GameObject _WebViewObject;

    private const float kSlowDriveSpeed_mmps = 30.0f;
    private const float kMediumDriveSpeed_mmps = 45.0f;
    private const float kFastDriveSpeed_mmps = 60.0f;
    private const float kDriveDist_mm = 44.0f; // length of one light cube
    private const float kTurnAngle = 90.0f * Mathf.Deg2Rad;

    protected override void InitializeGame(MinigameConfigBase minigameConfigData) {
      DAS.Debug("Loading Webview", "");
      UIManager.Instance.ShowTouchCatcher();

      LoadWebView();

      // TODO: call RaiseMiniGameQuit() when it's time to quit this game...
    }

    protected override void CleanUpOnDestroy() {
      // required function

      if (_WebViewObject != null) {

        GameObject.Destroy(_WebViewObject);
        _WebViewObject = null;
      }
      UIManager.Instance.HideTouchCatcher();
    }

    private void LoadWebView() {
      // Turn off music
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Silent);
      // Send EnterSDKMode to engine as we enter this view
      if (RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.EnterSDKMode(false);
      }

      if (_WebViewObject == null) {
        _WebViewObject = new GameObject("WebView", typeof(WebViewObject));
        WebViewObject webViewObjectComponent = _WebViewObject.GetComponent<WebViewObject>();
        webViewObjectComponent.Init(WebViewCallback, false, @"Mozilla/5.0 (iPhone; CPU iPhone OS 7_1_2 like Mac OS X) AppleWebKit/537.51.2 (KHTML, like Gecko) Version/7.0 Mobile/11D257 Safari/9537.53", WebViewError, WebViewLoaded, true);

#if UNITY_EDITOR
        string indexFile = Application.streamingAssetsPath + "/Scratch/index.html";
#else
        string indexFile = "file://" + PlatformUtil.GetResourcesBaseFolder() + "/Scratch/index.html";
#endif

        Debug.Log("Index file = " + indexFile);
        webViewObjectComponent.LoadURL(indexFile);
      }

      /*
      // TODO Consider using when close webview
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Freeplay);
      RobotEngineManager.Instance.CurrentRobot.ExitSDKMode();
      */
    }


    private void WebViewCallback(string text) {
      string jsonStringFromJS = string.Format("{0}", text);
      Debug.Log("JSON from JavaScript: " + jsonStringFromJS);
      ScratchRequest scratchRequest = JsonConvert.DeserializeObject<ScratchRequest>(jsonStringFromJS, GlobalSerializerSettings.JsonSettings);

      InProgressScratchBlock inProgressScratchBlock = InProgressScratchBlockPool.GetInProgressScratchBlock();
      inProgressScratchBlock.Init(scratchRequest.requestId, _WebViewObject.GetComponent<WebViewObject>());

      if (scratchRequest.command == "cozmoDriveForward") {
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
        RobotEngineManager.Instance.CurrentRobot.TurnInPlace(kTurnAngle, 0.0f, 0.0f, inProgressScratchBlock.AdvanceToNextBlock);
      }
      else if (scratchRequest.command == "cozmoTurnRight") {
        // Turn 90 degrees to the right
        RobotEngineManager.Instance.CurrentRobot.TurnInPlace(-kTurnAngle, 0.0f, 0.0f, inProgressScratchBlock.AdvanceToNextBlock);
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

        RobotEngineManager.Instance.CurrentRobot.SetLiftHeight(liftHeight, inProgressScratchBlock.AdvanceToNextBlock);
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
      case "happy":
        return Anki.Cozmo.AnimationTrigger.MeetCozmoFirstEnrollmentCelebration;
      case "victory":
        return Anki.Cozmo.AnimationTrigger.BuildPyramidSuccess;
      case "unhappy":
        return Anki.Cozmo.AnimationTrigger.FrustratedByFailureMajor;
      case "surprise":
        return Anki.Cozmo.AnimationTrigger.DroneModeTurboDrivingStart;
      case "dog":
        return Anki.Cozmo.AnimationTrigger.PetDetectionDog;
      case "cat":
        return Anki.Cozmo.AnimationTrigger.PetDetectionCat;
      case "sneeze":
        return Anki.Cozmo.AnimationTrigger.PetDetectionSneeze;
      case "excited":
        return Anki.Cozmo.AnimationTrigger.SuccessfulWheelie;
      case "thinking":
        return Anki.Cozmo.AnimationTrigger.HikingReactToPossibleMarker;
      case "bored":
        return Anki.Cozmo.AnimationTrigger.NothingToDoBoredEvent;
      case "frustrated":
        return Anki.Cozmo.AnimationTrigger.AskToBeRightedRight;
      case "chatty":
        return Anki.Cozmo.AnimationTrigger.BuildPyramidReactToBase;
      case "dejected":
        return Anki.Cozmo.AnimationTrigger.FistBumpLeftHanging;
      case "sleep":
        return Anki.Cozmo.AnimationTrigger.Sleeping;
      default:
        DAS.Error("Scratch.BadTriggerName", "Unexpected name '" + scratchAnimationName + "'");
        break;
      }
      return Anki.Cozmo.AnimationTrigger.MeetCozmoFirstEnrollmentCelebration;
    }

    private void WebViewError(string text) {
      Debug.LogError(string.Format("CallOnError[{0}]", text));
    }

    private void WebViewLoaded(string text) {
      Debug.Log(string.Format("CallOnLoaded[{0}]", text));
      WebViewObject webViewObjectComponent = _WebViewObject.GetComponent<WebViewObject>();
      webViewObjectComponent.SetVisibility(true);

#if !UNITY_ANDROID
      webViewObjectComponent.EvaluateJS(@"
              window.Unity = {
                call: function(msg) {
                  var iframe = document.createElement('IFRAME');
                  iframe.setAttribute('src', 'unity:' + msg);
                  document.documentElement.appendChild(iframe);
                  iframe.parentNode.removeChild(iframe);
                  iframe = null;
                }
              }
            ");
#endif
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
