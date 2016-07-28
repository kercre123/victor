using UnityEngine;
using System.Collections;

public class ConnectionFlowBackground : Cozmo.UI.BaseView {

  [SerializeField]
  private Sprite _CompletedStateSprite;

  [SerializeField]
  private Sprite _CompletedStateBackground;

  [SerializeField]
  private Sprite _InProgressSprite;

  [SerializeField]
  private Sprite _InProgressBackground;

  [SerializeField]
  private Sprite _FailedSprite;

  [SerializeField]
  private UnityEngine.UI.Image[] _StateImages;

  [SerializeField]
  private UnityEngine.UI.Image[] _StateBackgrounds;

  public void ResetAllProgress() {
    for (int i = 0; i < _StateImages.Length; ++i) {
      _StateImages[i].overrideSprite = null;
    }
    for (int i = 0; i < _StateBackgrounds.Length; ++i) {
      _StateBackgrounds[i].overrideSprite = null;
    }
  }

  public void SetStateInProgress(int currentState) {
    if (currentState < 0 || currentState >= _StateImages.Length) {
      DAS.Error("ConnectionFlowBackground.SetState", "Setting current state out of range");
    }
    _StateImages[currentState].overrideSprite = _InProgressSprite;
    _StateBackgrounds[currentState].overrideSprite = _InProgressBackground;
  }

  public void SetStateFailed(int failedState) {
    if (failedState < 0 || failedState >= _StateImages.Length) {
      DAS.Error("ConnectionFlowBackground.SetFailedState", "Setting current state out of range");
    }
    _StateImages[failedState].overrideSprite = _FailedSprite;
  }

  public void SetStateComplete(int completedState) {
    if (completedState < 0 || completedState >= _StateImages.Length) {
      DAS.Error("ConnectionFlowBackground.SetStateComplete", "Setting current state out of range");
    }
    _StateImages[completedState].overrideSprite = _CompletedStateSprite;
    _StateBackgrounds[completedState].overrideSprite = _CompletedStateBackground;
  }

  protected override void CleanUp() {

  }
}
