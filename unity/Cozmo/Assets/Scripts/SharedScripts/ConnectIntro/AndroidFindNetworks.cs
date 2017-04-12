using UnityEngine;

// Screen to wait on until we successfully scan a Cozmo to try to connect to
public class AndroidFindNetworks : AndroidConnectionFlowStage {

  // how long to wait before displaying the "Turn Cozmo on..." instructions
  private const float kInstructionsDelaySeconds = 5.0f;

  [SerializeField]
  private GameObject _SearchForCozmoFailedPrefab;

  private void Start() {
    StartCoroutine("UpdateNetworks");

    // register listeners to get scan results from Java, or be notified if we get a permissions issue
    var receiver = AndroidConnectionFlow.Instance.GetMessageReceiver();
    RegisterJavaListener(receiver, "scanResults", HandleScanResults);
    Invoke("ShowAnimations", kInstructionsDelaySeconds);
  }

  private void HandleScanResults(string[] ssids) {
    var cozmoNetworks = AndroidConnectionFlow.GetCozmoSSIDs(ssids);
    if (cozmoNetworks.Length > 0) {
      AndroidConnectionFlow.Instance.CozmoSSIDs = cozmoNetworks;
      OnStageComplete();
    }
  }

  private void ShowAnimations() {
    GameObject instructionsObject = GameObject.Instantiate(_SearchForCozmoFailedPrefab);
    instructionsObject.transform.SetParent(this.transform, false);
    var searchFailedScreen = instructionsObject.GetComponent<SearchForCozmoFailedScreen>();
    searchFailedScreen.OnQuitFlow += AndroidConnectionFlow.Instance.OnRestartFlow;
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
