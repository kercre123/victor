using UnityEngine;
using System.Collections;

public class SecuringConnectionScreen : MonoBehaviour {
  public System.Action OnScreenComplete;

  private void Start() {
    Invoke("ShowScreenComplete", ConnectionFlow.ConnectionFlowDelay);
  }

  private void ShowScreenComplete() {
    if (OnScreenComplete != null) {
      OnScreenComplete();
    }
  }
}
