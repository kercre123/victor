using UnityEngine;
using System.Collections;

public class PatternDiscoveredDialog : BaseDialog {

  [SerializeField]
  private PatternDiscoveryDisplay _patternDiscoveryDisplayPrefab;
  private PatternDiscoveryDisplay _patternDiscoveryDisplayInstance;

  public void Initialize(BlockPattern discoveredPattern) {
    GameObject display = UIManager.CreatePerspectiveUI (_patternDiscoveryDisplayPrefab.gameObject);
    _patternDiscoveryDisplayInstance = display.GetComponent<PatternDiscoveryDisplay> ();
    _patternDiscoveryDisplayInstance.Initialize (discoveredPattern);
  }

  public void OnCloseButtonTap() {
    GameObject.Destroy (_patternDiscoveryDisplayInstance.gameObject);
    UIManager.CloseDialog (this);
  }
}
