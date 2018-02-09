using UnityEngine;
using System;
using Cozmo.UI;

// Screen used to display status while waiting for wifi connection
// to complete
public class AndroidConnectToNetwork : AndroidConnectionFlowStage {

  // at least how many times should we attempt to connect, if at first we don't succeed?
  private const int kNumRetryAttempts = 3;
  // what's the minimum amount of time we should spend attempting to connect?
  private const double kMinFailureTimeSeconds = 10.0;
  // after how long should we give up on connecting, even if we haven't hit the total
  // number of retries yet?
  private const double kAutoFailureTimeSeconds = 30.0;

  [SerializeField]
  private CozmoText _SSIDLabel;

  [SerializeField]
  private CozmoText _StatusLabel;

  [SerializeField]
  private CozmoButton _CancelButton;

  private int _ConnectCount = 1;
  private DateTime _StartTime;

  private void Start() {

    // COZMO-14287: Catch & report unexpected System.ArgumentException
    try {
      DasTracker.Instance.TrackConnectFlowStarted();
    }
    catch (Exception ex) {
      DAS.Error("AndroidConnectToNetwork.Start.TrackConnectFlowStarted", ex.ToString());
    }


    _StartTime = DateTime.Now;

    try {
      _CancelButton.Initialize(AndroidConnectionFlow.Instance.UseOldFlow, "cancel_button", "android_connect_to_network");
    }
    catch (Exception ex) {
      DAS.Error("AndroidConnectToNetwork.Start.InitCancelButton", ex.ToString());
    }

    try {
      UpdateStatusLabels(CozmoBinding.CallWifiJava<string>("getCurrentSSID"), CozmoBinding.CallWifiJava<string>("getCurrentStatus"));
    }
    catch (Exception ex) {
      DAS.Error("AndroidConnectToNetwork.Start.UpdateStatusLabels", ex.ToString());
    }

    // register message listeners to get updates on when connection attempt completes, and when wifi status changes
    try {
      var receiver = AndroidConnectionFlow.Instance.GetMessageReceiver();
      RegisterJavaListener(receiver, "connectionFinished", HandleConnectionFinished);
      RegisterJavaListener(receiver, "wifiStatus", HandleWifiStatus);
    }
    catch (Exception ex) {
      DAS.Error("AndroidConnectToNetwork.Start.RegisterJavaListener", ex.ToString());
    }
  }

  private void HandleConnectionFinished(string[] args) {
    if (args[0] == "false") {
      // depending on whether password was the problem, either kick back to password screen
      // or try reconnecting
      // after a long enough time and enough failed attempts, give up
      if (CozmoBinding.CallWifiJava<bool>("didPasswordFail")) {
        AndroidConnectionFlow.Instance.HandleWrongPassword();
      }
      else {
        double secondsSinceStart = (System.DateTime.Now - _StartTime).TotalSeconds;
        if ((_ConnectCount >= kNumRetryAttempts && secondsSinceStart > kMinFailureTimeSeconds)
            || secondsSinceStart > kAutoFailureTimeSeconds) {
          _StatusLabel.text = "FAILED";
          DAS.Event("android_connect.failed", secondsSinceStart.ToString());
          Invoke("ConnectionFailure", 3.0f);
        }
        else {
          _ConnectCount++;
          bool result = CozmoBinding.CallWifiJava<bool>("reconnect", AndroidConnectionFlow.kTimeoutMs);
          DAS.Event(result ? "android_connect.retry" : "android_connect.retry_failed", _ConnectCount.ToString());
        }
      }
    }
    else {
      // success, this will auto close when ping test completes
    }
  }

  private void ConnectionFailure() {
    AndroidConnectionFlow.Instance.ConnectionFailed();
  }

  private void HandleWifiStatus(string[] args) {
    UpdateStatusLabels(args[0], args[1]);
  }

  private void UpdateStatusLabels(string ssid, string status) {
    if (Debug.isDebugBuild) {
      _SSIDLabel.text = Localization.GetWithArgs(LocalizationKeys.kWifiCurrentSsid, ssid);
      _StatusLabel.text = Localization.GetWithArgs(LocalizationKeys.kWifiCurrentStatus, status);
    }
    else {
      _SSIDLabel.gameObject.SetActive(false);
      _StatusLabel.gameObject.SetActive(false);
    }
  }
}
