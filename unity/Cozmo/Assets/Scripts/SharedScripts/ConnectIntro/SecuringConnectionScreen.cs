using UnityEngine;
using System.Collections;

public class SecuringConnectionScreen : MonoBehaviour {
  public System.Action OnScreenComplete;

  private void Start() {
    Invoke("ShowScreenComplete", ConnectionFlowController.ConnectionFlowDelay);
  }

  private void ShowScreenComplete() {
    if (OnScreenComplete != null) {
      OnScreenComplete();
    }
  }
}
