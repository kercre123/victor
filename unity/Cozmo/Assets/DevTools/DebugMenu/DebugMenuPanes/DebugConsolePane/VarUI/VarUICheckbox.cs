using UnityEngine;
using UnityEngine.UI;

namespace Anki.Debug {
  public class VarUICheckbox : ConsoleVarLine {

    [SerializeField]
    private Toggle _Checkbox;

    public override void Init(DebugConsoleData.DebugConsoleVarData singleVar, GameObject go) {
      base.Init(singleVar, go);
      OnValueRefreshed();
      _Checkbox.onValueChanged.AddListener(HandleValueChanged);
    }

    public override void OnValueRefreshed() {
      _Checkbox.isOn = _VarData.ValueAsUInt64 != 0;
      if (_VarData.UnityObject != null) {
        _Checkbox.isOn = (bool)GetUnityValue();
      }
    }

    private void HandleValueChanged(bool val) {
      // If the game is fine with this value it will send a VerifyDebugConsoleVarMessage
      // otherwise it will send another Set to a valid value.
      // Empty string just means toggle.
      if (_VarData.UnityObject != null) {
        SetUnityValue(val);
      }
      else {
        RobotEngineManager.Instance.SetDebugConsoleVar(_VarData.VarName, "");
      }
    }
  }
}
