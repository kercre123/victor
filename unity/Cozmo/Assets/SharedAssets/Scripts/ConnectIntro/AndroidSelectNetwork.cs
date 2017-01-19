using Anki.UI;
using Cozmo.UI;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

// If multiple Cozmos are detected, or if the user indicates that the
// Cozmo displayed on the Enter Password screen is the wrong one, this
// screen allows the user to select which Cozmo they'd like to connect to
//
// This screen also runs a coroutine in the background that continues
// scanning for new networks and updates the list of available Cozmos
// accordingly
public class AndroidSelectNetwork : AndroidConnectionFlowStage {

  [SerializeField]
  private AnkiTextLabel _InstructionsLabel;

  [SerializeField]
  private CozmoButton _ContinueButton;

  [SerializeField]
  private AndroidNetworkCell _NetworkCellPrefab;

  [SerializeField]
  private RectTransform _ScrollViewContent;

  private List<string> _CurrentNetworks = new List<string>();

  private AndroidNetworkCell _SelectedCell;

  private void Start() {
    var networks = AndroidConnectionFlow.Instance.CozmoSSIDs;
    AddNetworkCells(networks);

    StartCoroutine("UpdateNetworks");
    _ContinueButton.Initialize(HandleContinueButton, "continue_button", "android_select_network");
    _ContinueButton.Interactable = false;

    var receiver = AndroidConnectionFlow.Instance.GetMessageReceiver();
    RegisterJavaListener(receiver, "scanResults", HandleScanResults);
  }

  private void AddNetworkCells(string[] networks) {
    foreach (string network in networks) {
      if (!_CurrentNetworks.Contains(network)) {
        _CurrentNetworks.Add(network);
        AndroidNetworkCell cell = GameObject.Instantiate(_NetworkCellPrefab.gameObject).GetComponent<AndroidNetworkCell>();
        cell.Init(network, () => {
          if (_SelectedCell != null) {
            _SelectedCell.Deselect();
          }
          cell.Select();
          _SelectedCell = cell;
          _ContinueButton.Interactable = true;
        });
        cell.transform.SetParent(_ScrollViewContent, false);
      }
    }
  }

  private void HandleScanResults(string[] ssids) {
    AddNetworkCells(AndroidConnectionFlow.GetCozmoSSIDs(ssids));
  }

  private void HandleContinueButton() {
    string selection = _SelectedCell != null ? _SelectedCell.text : null;
    if (string.IsNullOrEmpty(selection)) {
      return;
    }

    AndroidConnectionFlow.Instance.SelectedSSID = selection;
    StopCoroutine("UpdateNetworks");
    OnStageComplete();
  }

  private System.Collections.IEnumerator UpdateNetworks() {
    while (true) {
      AndroidConnectionFlow.CallJava<bool>("requestScan");
      yield return new WaitForSeconds(3.0f);
    }
  }

}
