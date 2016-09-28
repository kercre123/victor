using UnityEngine;
using UnityEngine.UI;
using Anki.Cozmo;

namespace Anki.Debug {
  public class VarUIText : ConsoleVarLine {

    [SerializeField]
    private InputField _StatInputField;

    public override void Init(DebugConsoleData.DebugConsoleVarData singleVar, GameObject go) {
      base.Init(singleVar, go);
      OnValueRefreshed();
      _StatInputField.onValueChanged.AddListener(HandleValueChanged);
    }

    public override void OnValueRefreshed() {
      switch (_VarData.TagType) {
      case ConsoleVarUnion.Tag.varDouble:
        _StatInputField.text = _VarData.ValueAsDouble.ToString();
        break;
      case ConsoleVarUnion.Tag.varInt:
        _StatInputField.text = _VarData.ValueAsInt64.ToString();
        break;
      case ConsoleVarUnion.Tag.varUint:
        _StatInputField.text = _VarData.ValueAsUInt64.ToString();
        break;
      }

      if (_VarData.UnityObject != null) {
        _StatInputField.text = GetUnityValue().ToString();
      }
    }

    private void HandleValueChanged(string strValue) {
      // If it's a unity variable, pass back with the same type.
      if (_VarData.UnityObject != null) {
        switch (_VarData.TagType) {
        case ConsoleVarUnion.Tag.varDouble: {
            float result;
            if (float.TryParse(strValue, out result)) {
              SetUnityValue(result);
            }
          }
          break;
        case ConsoleVarUnion.Tag.varInt: {
            int result;
            if (int.TryParse(strValue, out result)) {
              SetUnityValue(result);
            }
          }
          break;
        }
      }
      // If the game is fine with this value it will send a VerifyDebugConsoleVarMessage
      // otherwise it will send another Set to a valid value.
      else {
        RobotEngineManager.Instance.SetDebugConsoleVar(_VarData.VarName, strValue);
      }
    }
  }
}
