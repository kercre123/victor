using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class UnlockDebugPane : MonoBehaviour {
  [SerializeField]
  private UnityEngine.UI.Button _LockButton;

  [SerializeField]
  private UnityEngine.UI.Button _UnlockButton;

  [SerializeField]
  private UnityEngine.UI.Dropdown _UnlockSelection;

  private void Start() {
    _LockButton.onClick.AddListener(OnHandleLockButtonClicked);
    _UnlockButton.onClick.AddListener(OnHandleUnlockButtonClicked);
    PopulateOptions();
  }

  private void PopulateOptions() {
    List<UnityEngine.UI.Dropdown.OptionData> options = new List<UnityEngine.UI.Dropdown.OptionData>();
    for (int i = 0; i < System.Enum.GetValues(typeof(Anki.Cozmo.UnlockIds)).Length; ++i) {
      UnityEngine.UI.Dropdown.OptionData option = new UnityEngine.UI.Dropdown.OptionData();
      option.text = System.Enum.GetValues(typeof(Anki.Cozmo.UnlockIds)).GetValue(i).ToString();
      options.Add(option);
    }
    _UnlockSelection.AddOptions(options);
  }

  private Anki.Cozmo.UnlockIds GetSelectedUnlockId() {
    return (Anki.Cozmo.UnlockIds)System.Enum.Parse(typeof(Anki.Cozmo.UnlockIds), _UnlockSelection.options[_UnlockSelection.value].text);
  }

  private void OnHandleLockButtonClicked() {
    UnlockablesManager.Instance.TrySetUnlocked(GetSelectedUnlockId(), false);
  }

  private void OnHandleUnlockButtonClicked() {
    UnlockablesManager.Instance.TrySetUnlocked(GetSelectedUnlockId(), true);
  }
}
