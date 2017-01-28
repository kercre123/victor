using Anki.UI;
using Cozmo.UI;
using UnityEngine;
using System;

// Screen to request permissions from the OS or user actions that we need
// in order to get wifi scan results

// This screen goes in order through a few things that we need and determines:
// - do we need to ask for this
// - if so, ask the user for it and detect if they said yes or no
// - if yes, move to the next thing
// - be annoying and ask them again if they say no
// - if they really don't want to do it, then kick them to the old flow and
//   disable the super duper android flow from running again in the future
public class AndroidGetPermissions : AndroidConnectionFlowStage {

  [SerializeField]
  private AnkiTextLabel _Instructions;

  [SerializeField]
  private CozmoButton _ExecuteButton;

  [SerializeField]
  private CozmoButton _CancelButton;

  private void Start() {
    _CancelButton.Initialize(AndroidConnectionFlow.Instance.UseOldFlow, "cancel_button", "android_get_permissions");
    CheckWifiEnabled();
  }

  private void InitStage(string instructionsLocalizationKey, UnityEngine.Events.UnityAction buttonCallback) {
    _ExecuteButton.onClick.RemoveAllListeners();
    _ExecuteButton.Initialize(buttonCallback, "ok_button", "android_get_permissions");
    _Instructions.text = Localization.Get(instructionsLocalizationKey);
  }

  private void CheckWifiEnabled() {
    Action nextStage = CheckLocationPermission;
    bool changeNeeded = !AndroidConnectionFlow.CallJava<bool>("isWifiEnabled");
    if (!changeNeeded) {
      nextStage();
      return;
    }

    InitStage(LocalizationKeys.kWifiEnableWifi, () => {
      AndroidConnectionFlow.CallJava("enableWifi");
      nextStage();
    });
  }

  private void CheckLocationPermission() {
    Action<bool> nextStage = CheckLocationEnabled;
    bool changeNeeded = !AndroidConnectionFlow.CallJava<bool>("hasLocationPermission");
    if (!changeNeeded) {
      nextStage(false);
      return;
    }

    Action successAction = () => nextStage(true);

    InitStage(LocalizationKeys.kWifiLocationPermission, () => {
      SetWaitingLabel();
      RegisterJavaListener(AndroidConnectionFlow.Instance.GetMessageReceiver(), "permissionResult",
        args => ParseJavaResult(args, successAction, () => HandlePermissionRejected(successAction)));
      AndroidConnectionFlow.CallJava("requestLocationPermission");
    });
  }

  private void HandlePermissionRejected(Action successAction) {
    // they said no. did they check the "never ask again" box?
    bool userHatesFun = !CozmoBinding.GetCurrentActivity().Call<bool>("shouldShowRequestPermissionRationale",
                                                                      "android.permission.ACCESS_COARSE_LOCATION");
    if (userHatesFun) {
      OnCatastrophicFailure();
      return;
    }

    // ask again nicely, this time just bail if they still say no
    ClearJavaListeners();
    InitStage(LocalizationKeys.kWifiPleaseLocationPermission, () => {
      SetWaitingLabel();
      RegisterJavaListener(AndroidConnectionFlow.Instance.GetMessageReceiver(), "permissionResult",
        args => ParseJavaResult(args, successAction, OnCatastrophicFailure));
      AndroidConnectionFlow.CallJava("requestLocationPermission");
    });
  }

  private void SetWaitingLabel() {
    _Instructions.text = Localization.Get(LocalizationKeys.kLabelWaiting);
  }

  private void CheckLocationEnabled(bool didGetPermissions) {
    Action nextStage = () => OnStageComplete();
    bool changeNeeded = AndroidConnectionFlow.CallJava<bool>("needLocationService");
    if (!changeNeeded) {
      nextStage();
      return;
    }

    Action stageAction = () => {
      RegisterJavaListener(AndroidConnectionFlow.Instance.GetMessageReceiver(), "locationResult",
        args => ParseJavaResult(args, nextStage, () => HandleLocationRejected(nextStage)));
      RequestEnableLocation();
    };

    // if we didn't ask the user for location permissions, show the UI via
    // the InitStage() call. if we DID just ask the user for location permissions,
    // don't bother with new UI and just ask them immediately
    if (didGetPermissions) {
      stageAction();
    } else {
      InitStage(LocalizationKeys.kWifiEnableLocation, () => {
        SetWaitingLabel();
        stageAction();
      });
    }
  }

  private void HandleLocationRejected(Action successAction) {
    // ask again nicely, this time just bail if they still say no
    ClearJavaListeners();
    InitStage(LocalizationKeys.kWifiPleaseEnableLocation, () => {
      SetWaitingLabel();
      RegisterJavaListener(AndroidConnectionFlow.Instance.GetMessageReceiver(), "locationResult",
        args => ParseJavaResult(args, successAction, AndroidConnectionFlow.Instance.UseOldFlow));
      RequestEnableLocation();
    });
  }

  private void RequestEnableLocation() {
    var activity = CozmoBinding.GetCurrentActivity();
    AndroidConnectionFlow.CallLocationJava("requestEnableLocation", activity, activity.Call<AndroidJavaObject>("getDispatcher"));
  }

  private void OnCatastrophicFailure() {
    AnkiPrefs.SetInt(AnkiPrefs.Prefs.AndroidAutoConnectDisabled, 1);
    AndroidConnectionFlow.Instance.UseOldFlow();
  }

  private void ParseJavaResult(string[] args, Action onSuccess, Action onFail) {
    bool result = Boolean.Parse(args[0]);
    if (result) {
      onSuccess();
    }
    else {
      onFail();
    }
  }
}
