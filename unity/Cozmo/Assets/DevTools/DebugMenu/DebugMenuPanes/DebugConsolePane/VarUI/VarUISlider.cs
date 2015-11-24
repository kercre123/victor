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

      _Slider.minValue = (float)singleVar.MinValue;
      _Slider.maxValue = (float)singleVar.MaxValue;
      float setVal = (float)singleVar.ValueAsDouble;
      switch (singleVar.TagType) {
      case ConsoleVarUnion.Tag.varInt:
        setVal = (float)singleVar.ValueAsInt64;
        break;
      case ConsoleVarUnion.Tag.varUint:
        setVal = (float)singleVar.ValueAsUInt64;
        break;
      }
      _Slider.value = (float)setVal;
      SetSliderVal(_Slider.value);

      _Slider.onValueChanged.AddListener(HandleValueChanged);
    }

    private void HandleValueChanged(float val) {
      // If the game is fine with this value it will send a VerifyDebugConsoleVarMessage
      // otherwise it will send another Set to a valid value.
      if (_VarData.UnityVarHandler != null) {
        _VarData.UnityVarHandler(val);
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
