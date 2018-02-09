using UnityEngine;
using System.Collections;

public class SearchForCozmoScreen : MonoBehaviour {
  public System.Action<bool> OnScreenComplete;
  private PingStatus _PingStatus;


  [SerializeField]
  private int _AttemptsBeforeShowingFailScreen = 2;
  private int _PingCheckAttempts = 0;

#if UNITY_ANDROID
  private const int _kMaxAndroid8ConnectionAttempts = 10;
  private Coroutine _Android8ConnectivityCoroutine;
#endif
  
  public void Initialize(PingStatus pingStatus) {
    _PingStatus = pingStatus;
  }

  private void Start() {
#if  UNITY_ANDROID
    if (CozmoBinding.IsOreoOrNewer()) {
      _Android8ConnectivityCoroutine = StartCoroutine(CheckAndroid8Connectivity());
    }
#endif
    
    Invoke("CheckForConnection", ConnectionFlowController.ConnectionFlowDelay);
  }

  private void CheckForConnection() {
    if (_PingStatus == null) {
      // we weren't given a ping object, which means another component will
      // be tasked with determining when we're connected
      return;
    }

    _PingCheckAttempts++;
    if (_PingStatus.GetPingStatus() || _PingCheckAttempts >= _AttemptsBeforeShowingFailScreen) {
      ShowScreenComplete(_PingStatus.GetPingStatus());
    }
    else {
      Invoke("CheckForConnection", ConnectionFlowController.ConnectionFlowDelay);
    }
  }


  // If we use the Ping provided by Unity (as in CheckForConnection()) when the user has both a Wifi and data connection,
  // the ping fails because it isn't isolated to Wifi. The ping provided in the cozmojava native lib is guaranteed
  // to go over Wifi, so we use that here instead.
#if UNITY_ANDROID
  private System.Collections.IEnumerator CheckAndroid8Connectivity() {
    while (_PingCheckAttempts < _kMaxAndroid8ConnectionAttempts) {
      if (AndroidConnectionFlow.IsPingSuccessful()) {
        DAS.Info("AndroidConnectionFlow.CheckConnectivity", "ping succeeded, exiting");
        ShowScreenComplete(true);
        yield break;
      }

      _PingCheckAttempts++;
      yield return new WaitForSeconds(0.5f);
    }

    ShowScreenComplete(false);
  }
#endif
  
  private void ShowScreenComplete(bool pingSucceeded) {
#if UNITY_ANDROID
    if (_Android8ConnectivityCoroutine != null) {
      StopCoroutine(_Android8ConnectivityCoroutine);
    }

    AndroidConnectionFlow.StopPingTest();
#endif

    OnScreenComplete(pingSucceeded);
  }
}