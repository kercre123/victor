using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class PlaybackManager : MonoBehaviour {
  // Use this for initialization
  void Start() {
    if (Anki.Core.UI.Automation.Automation.Instance == null) {
      new GameObject("Anki.Core.UI.Automation.Automation", typeof(Anki.Core.UI.Automation.Automation));
    }
  }
}
