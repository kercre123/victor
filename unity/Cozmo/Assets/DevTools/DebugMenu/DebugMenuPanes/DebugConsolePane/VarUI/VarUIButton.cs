using UnityEngine;
using UnityEngine.UI;

namespace Anki.Debug {
  public class VarUIButton : ConsoleVarLine {

    [SerializeField]
    private Button _Button;

    [SerializeField]
    private InputField _StatInputField;

    public override void Init(DebugConsoleData.DebugConsoleVarData singleVar, GameObject go) {
      base.Init(singleVar, go);

      _Button.onClick.AddListener(HandleClick);
    }

    public void HandleClick() {
      if (_VarData.UnityVarHandler != null) {
        // don't send null
        _VarData.UnityVarHandler(_StatInputField.text);
      }
      else {
        RobotEngineManager.Instance.RunDebugConsoleFuncMessage(_VarData.VarName, _StatInputField.text);
      }
    }

  }
}
