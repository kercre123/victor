using UnityEngine;
using System.Collections;

public class PressDemoView : Cozmo.UI.BaseView {

  public System.Action OnForceProgress;

  [SerializeField]
  private UnityEngine.UI.Button _ForceProgressButton;

  void Start() {
    _ForceProgressButton.onClick.AddListener(HandeForceProgressPressed);
  }

  private void HandeForceProgressPressed() {
    if (OnForceProgress != null) {
      OnForceProgress();
    }
  }

  protected override void CleanUp() {
    
  }
}
