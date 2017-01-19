using UnityEngine;

// Screen to wait on until we successfully scan a Cozmo to try to connect to
public class AndroidFindNetworks : AndroidConnectionFlowStage {

  [SerializeField]
  private Anki.UI.AnkiButton _CancelButton;

  [SerializeField]
  private GameObject _WifiAnimationsPrefab;

  private void Start() {
    StartCoroutine("UpdateNetworks");
    _CancelButton.Initialize(AndroidConnectionFlow.Instance.UseOldFlow, "cancel", "android_find_networks");

    // register listeners to get scan results from Java, or be notified if we get a permissions issue
    var receiver = AndroidConnectionFlow.Instance.GetMessageReceiver();
    RegisterJavaListener(receiver, "scanResults", HandleScanResults);
    Invoke("ShowAnimations", 2.5f);
  }

  private void HandleScanResults(string[] ssids) {
    var cozmoNetworks = AndroidConnectionFlow.GetCozmoSSIDs(ssids);
    if (cozmoNetworks.Length > 0) {
      AndroidConnectionFlow.Instance.CozmoSSIDs = cozmoNetworks;
      OnStageComplete();
    }
  }

  private void ShowAnimations() {
    GameObject wifiAnimations = GameObject.Instantiate(_WifiAnimationsPrefab);
    wifiAnimations.transform.SetParent(this.transform, false);
  }

  // coroutine to request a new wifi scan every few seconds
  // while this screen is active
  private System.Collections.IEnumerator UpdateNetworks() {
    while (true) {
      AndroidConnectionFlow.CallJava<bool>("requestScan");
      yield return new WaitForSeconds(3.0f);
    }
  }

}
