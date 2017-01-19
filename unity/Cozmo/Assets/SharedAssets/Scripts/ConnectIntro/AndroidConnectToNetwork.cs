using Anki.UI;
using UnityEngine;
using System;

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
  private AnkiTextLabel _SSIDLabel;

  [SerializeField]
  private AnkiTextLabel _StatusLabel;

  [SerializeField]
  private AnkiTextLabel _ConnectingLabel;

  [SerializeField]
  private AnkiButton _TryAgainButton;

  [SerializeField]
  private AnkiButton _RestartButton;

  private int _ConnectCount = 1;
  private int _NumDots = 1;
  private string _ConnectingBaseText;
  private DateTime _StartTime;

  private void Start() {
    DasTracker.Instance.TrackConnectFlowStarted();
    _StartTime = DateTime.Now;

    _TryAgainButton.Initialize(() => {
      var SSID = AndroidConnectionFlow.Instance.SelectedSSID;
      bool result = AndroidConnectionFlow.CallJava<bool>("reconnect", AndroidConnectionFlow.kTimeoutMs);
      DAS.Info("AndroidConnectToNetwork", "Retrying connection to " + SSID + ", result = " + result);
    }, "retry_button", "android_connect_to_network");

    _RestartButton.Initialize(AndroidConnectionFlow.Instance.RestartFlow, "restart_button", "android_connect_to_network");

    UpdateStatusLabels(AndroidConnectionFlow.CallJava<string>("getCurrentSSID"), AndroidConnectionFlow.CallJava<string>("getCurrentStatus"));

    // register message listeners to get updates on when connection attempt completes, and when wifi status changes
    var receiver = AndroidConnectionFlow.Instance.GetMessageReceiver();
    RegisterJavaListener(receiver, "connectionFinished", HandleConnectionFinished);
    RegisterJavaListener(receiver, "wifiStatus", HandleWifiStatus);

    _ConnectingBaseText = Localization.Get(_ConnectingLabel.text);
    StartCoroutine("ConnectingDots");
  }

  private void HandleConnectionFinished(string[] args) {
    if (args[0] == "false") {
      // depending on whether password was the problem, either kick back to password screen
      // or try reconnecting
      // after a long enough time and enough failed attempts, give up
      if (AndroidConnectionFlow.CallJava<bool>("didPasswordFail")) {
        AndroidConnectionFlow.Instance.HandleWrongPassword();
      } else {
        double secondsSinceStart = (System.DateTime.Now - _StartTime).TotalSeconds;
        if (  (_ConnectCount >= kNumRetryAttempts && secondsSinceStart > kMinFailureTimeSeconds)
            || secondsSinceStart > kAutoFailureTimeSeconds) {
          _StatusLabel.text = "FAILED";
          DAS.Event("android_connect.failed", secondsSinceStart.ToString());
          Invoke("ConnectionFailure", 3.0f);
        } else {
          _ConnectCount++;
          bool result = AndroidConnectionFlow.CallJava<bool>("reconnect", AndroidConnectionFlow.kTimeoutMs);
          DAS.Event(result ? "android_connect.retry" : "android_connect.retry_failed", _ConnectCount.ToString());
        }
      }
    } else {
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
    _SSIDLabel.text = string.Format(Localization.Get("wifi.currentSsid"), ssid);
    _StatusLabel.text = string.Format(Localization.Get("wifi.currentStatus"), status);
  }

  private System.Collections.IEnumerator ConnectingDots() {
    while (true) {
      int renderDots = Math.Min(_NumDots, 3); // clamp to 3 dots
      _ConnectingLabel.text = _ConnectingBaseText + new string('.', renderDots);
      yield return new WaitForSeconds(0.5f);
      _NumDots = (_NumDots % 4) + 1; // cycle from 1...4 (displays 3 dots for two cycles)
    }
  }
}
