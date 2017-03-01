using Anki.UI;
using Cozmo.UI;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Minigame.DroneMode {
  public class DroneModeActionButton : MonoBehaviour {
    [SerializeField]
    private CozmoButtonLegacy _ActionButton;
    public bool Interactable {
      get { return _ActionButton.Interactable; }
      set { _ActionButton.Interactable = value; }
    }

    private bool _NeedsCubeSeen;
    public bool NeedsCubeSeen { get { return _NeedsCubeSeen; } }

    private bool _NeedsFaceSeen;
    public bool NeedsFaceSeen { get { return _NeedsFaceSeen; } }

    private System.Action _ActionCallback;
    private AlertModal _AlertModal;

    public void Initialize(string dasButtonName, string dasViewController, DroneModeActionData actionData,
                           System.Action actionCallback, bool needsCubeSeen, bool needsFaceSeen) {
      _ActionButton.Initialize(HandleButtonClicked, dasButtonName, dasViewController);
      _ActionButton.Text = Localization.Get(actionData.ButtonTextLocKey);
      _ActionCallback = actionCallback;
      _NeedsCubeSeen = needsCubeSeen;
      _NeedsFaceSeen = needsFaceSeen;
    }

    private void HandleButtonClicked() {
      if (_ActionCallback != null) {
        _ActionCallback();
      }
    }
  }
}