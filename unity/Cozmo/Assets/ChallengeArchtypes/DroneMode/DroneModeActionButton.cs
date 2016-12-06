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
    private GameObject _LockedIcon;

    private bool _IsUnlocked;
    public bool IsUnlocked { get { return _IsUnlocked; } }

    private string _UpgradeName;

    private bool _NeedsCubeSeen;
    public bool NeedsCubeSeen { get { return _NeedsCubeSeen; } }

    private bool _NeedsFaceSeen;
    public bool NeedsFaceSeen { get { return _NeedsFaceSeen; } }

    private System.Action _ActionCallback;
    private AlertModal _AlertView;

    public void Initialize(string dasButtonName, string dasViewController, DroneModeActionData actionData,
                           System.Action actionCallback, bool needsCubeSeen, bool needsFaceSeen) {
      _ActionButton.Initialize(HandleButtonClicked, dasButtonName, dasViewController);
      _ActionButton.Text = Localization.Get(actionData.ButtonTextLocKey);
      _ActionCallback = actionCallback;
      _NeedsCubeSeen = needsCubeSeen;
      _NeedsFaceSeen = needsFaceSeen;

      _IsUnlocked = ((actionData.ActionUnlockId == Anki.Cozmo.UnlockId.Invalid) || UnlockablesManager.Instance.IsUnlocked(actionData.ActionUnlockId));
      _LockedIcon.SetActive(!_IsUnlocked);
      _ActionButton.Interactable = _IsUnlocked;
      _ActionButton.ShowDisabledStateWhenInteractable = !_IsUnlocked;

      if (actionData.ActionUnlockId != Anki.Cozmo.UnlockId.Invalid) {
        UnlockableInfo upgradeInfo = UnlockablesManager.Instance.GetUnlockableInfo(actionData.ActionUnlockId);
        if (upgradeInfo != null) {
          _UpgradeName = Localization.Get(upgradeInfo.TitleKey);
        }
      }
    }

    private void HandleButtonClicked() {
      if (_IsUnlocked) {
        if (_ActionCallback != null) {
          _ActionCallback();
        }
      }
      else {
        if (_AlertView == null) {
          _AlertView = UIManager.OpenModal(AlertModalLoader.Instance.AlertModalPrefab);
          // Hook up callbacks
          _AlertView.SetCloseButtonEnabled(true);
          _AlertView.SetPrimaryButton(LocalizationKeys.kButtonOkay);

          _AlertView.TitleLocKey = LocalizationKeys.kDroneModeActionLockedModalTitle;
          _AlertView.DescriptionLocKey = LocalizationKeys.kDroneModeActionLockedModalDescription;
          _AlertView.SetMessageArgs(new object[] { _UpgradeName });
        }
      }
    }
  }
}