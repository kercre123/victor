using UnityEngine;
using System.Collections;

public class PatternDiscoveredDialog : BaseDialog {
  public void Initialize(BlockPattern discoveredPattern) {
  }

  public void OnCloseButtonTap() {
    UIManager.CloseDialog (this);
  }
}
