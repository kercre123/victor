using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

namespace Anki.Debug {
  public class VarUISlider : ConsoleVarLine {

    [SerializeField]
    private Slider _Slider;

    [SerializeField]
    private Text _SliderLabel;

    public override void Init(DebugConsoleData.DebugConsoleVarData singleVar) {
      base.Init(singleVar);

      _Slider.minValue = (float)singleVar._minValue;
      _Slider.maxValue = (float)singleVar._maxValue;
      float setVal = (float)singleVar._valueAsDouble;
      switch (singleVar._tagType) {
      case ConsoleVarUnion.Tag.varInt:
        setVal = (float)singleVar._valueAsInt64;
        break;
      case ConsoleVarUnion.Tag.varUint:
        setVal = (float)singleVar._valueAsUInt64;
        break;
      }
      _Slider.value = (float)setVal;
      SetSliderVal(_Slider.value);

      _Slider.onValueChanged.AddListener(HandleValueChanged);
    }

    private void HandleValueChanged(float val) {
      // If the game is fine with this value it will send a VerifyDebugConsoleVarMessage
      // otherwise it will send another Set to a valid value.
      if (_varData._unityVarHandler != null) {
        _varData._unityVarHandler(val);
      }
      else {
        RobotEngineManager.Instance.SetDebugConsoleVar(_varData._varName, val.ToString());
      }
      SetSliderVal(val);
    }

    public void SetSliderVal(double val) {
      _SliderLabel.text = val.ToString("N2");
    }
  }
}
