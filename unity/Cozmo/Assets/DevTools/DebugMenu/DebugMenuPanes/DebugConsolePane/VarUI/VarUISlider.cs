using UnityEngine;
using UnityEngine.UI;
using Anki.Cozmo;

namespace Anki.Debug {
  public class VarUISlider : ConsoleVarLine {

    [SerializeField]
    private Slider _Slider;

    [SerializeField]
    private Text _SliderLabel;

    public override void Init(DebugConsoleData.DebugConsoleVarData singleVar, GameObject go) {
      base.Init(singleVar, go);

      _Slider.minValue = (float)singleVar.MinValue;
      _Slider.maxValue = (float)singleVar.MaxValue;
      OnValueRefreshed();

      _Slider.onValueChanged.AddListener(HandleValueChanged);
    }

    public override void OnValueRefreshed() {
      float setVal = (float)_VarData.ValueAsDouble;
      switch (_VarData.TagType) {
      case ConsoleVarUnion.Tag.varInt:
        setVal = (float)_VarData.ValueAsInt64;
        break;
      case ConsoleVarUnion.Tag.varUint:
        setVal = (float)_VarData.ValueAsUInt64;
        break;
      }
      if (_VarData.UnityObject != null) {
        setVal = (float)GetUnityValue();
      }
      _Slider.value = setVal;
      SetSliderVal(_Slider.value);
    }

    private void HandleValueChanged(float val) {
      // If the game is fine with this value it will send a VerifyDebugConsoleVarMessage
      // otherwise it will send another Set to a valid value.
      if (_VarData.UnityObject != null) {
        SetUnityValue(val);
      }
      else {
        RobotEngineManager.Instance.SetDebugConsoleVar(_VarData.VarName, val.ToString());
      }
      SetSliderVal(val);
    }

    public void SetSliderVal(double val) {
      _SliderLabel.text = val.ToString("N2");
    }
  }
}
