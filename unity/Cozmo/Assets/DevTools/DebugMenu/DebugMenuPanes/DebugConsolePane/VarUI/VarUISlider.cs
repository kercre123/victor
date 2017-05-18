using UnityEngine;
using UnityEngine.UI;
using Anki.Cozmo;
using UnityEngine.EventSystems;// Required when using Event data.

namespace Anki.Debug {

  // unity pointer up is a function that needs to be directly on the UI object, not a callback event.
  public class SliderEventListenerHelper : MonoBehaviour, IPointerUpHandler {
    public VarUISlider consoleWrapper;
    public void OnPointerUp(PointerEventData eventData) {
      consoleWrapper.SendUpdateToEngine();
    }
  }

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
      // Just set when no longer touching, but visually update.
      // if we update every set it causes a lot of engine spamming and jumping around
      SliderEventListenerHelper helper = _Slider.gameObject.AddComponent<SliderEventListenerHelper>();
      helper.consoleWrapper = this;
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

    public void SendUpdateToEngine() {
      float val = _Slider.value;
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

    private void HandleValueChanged(float val) {
      SetSliderVal(val);
    }

    public void SetSliderVal(double val) {
      _SliderLabel.text = val.ToString("N2");
    }
  }
}
