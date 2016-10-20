using Anki.UI;
using Cozmo.UI;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo.Minigame.DroneMode {
  public class DroneModeActionButton : MonoBehaviour {
    [SerializeField]
    private CozmoButton _ActionButton;
    public bool Interactable {
      get { return _ActionButton.Interactable; }
      set { _ActionButton.Interactable = value; }
    }

    [SerializeField]
    private Image _ActionIcon;

    [SerializeField]
    private GameObject _LockedIcon;

    private bool _IsUnlocked;
    public bool IsUnlocked { get { return _IsUnlocked; } }

    private bool _NeedsCubeSeen;
    public bool NeedsCubeSeen { get { return _NeedsCubeSeen; } }

    private System.Action _ActionCallback;
    private AlertView _AlertView;

    public void Initialize(string dasButtonName, string dasViewController, DroneModeActionData actionData, System.Action actionCallback, bool needsCubeSeen) {
      _ActionIcon.sprite = actionData.ButtonIcon;
      _ActionButton.Initialize(HandleButtonClicked, dasButtonName, dasViewController);
      _ActionButton.Text = actionData.ButtonText;
      _ActionCallback = actionCallback;
      _NeedsCubeSeen = needsCubeSeen;

      _IsUnlocked = ((actionData.ActionUnlockId == Anki.Cozmo.UnlockId.Invalid) || UnlockablesManager.Instance.IsUnlocked(actionData.ActionUnlockId));
      _LockedIcon.SetActive(!_IsUnlocked);
      _ActionButton.Interactable = _IsUnlocked;
    }

    private void HandleButtonClicked() {
      if (_IsUnlocked) {
        if (_ActionCallback != null) {
          _ActionCallback();
        }
      }
      else {
        if (_AlertView == null) {
          _AlertView = UIManager.OpenView(AlertViewLoader.Instance.AlertViewPrefab);
          // Hook up callbacks
          _AlertView.SetCloseButtonEnabled(true);
          _AlertView.SetPrimaryButton(LocalizationKeys.kButtonContinue);

          // TODO: Real localization
          _AlertView.TitleLocKey = LocalizationKeys.kChallengeDetailsNeedsMoreCubesModalTitle;
          _AlertView.DescriptionLocKey = LocalizationKeys.kChallengeDetailsNeedsMoreCubesModalDescription;
        }
      }
    }
  }
}