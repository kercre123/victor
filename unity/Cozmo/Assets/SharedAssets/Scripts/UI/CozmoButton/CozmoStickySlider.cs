using UnityEngine;
using UnityEngine.UI;

namespace Cozmo {
  namespace UI {
    public class CozmoStickySlider : Slider {
      [SerializeField, Range(0f, 1f)]
      private float _RestingSliderValue;

      [SerializeField, Range(0f, 1f)]
      private float _SliderValueDecayRate = 0.9f;

      [SerializeField]
      private bool _SnapBackInstantly = true;

      private bool _IsTouching;

      // Use this for initialization
      protected override void Start() {
        base.Start();
        _IsTouching = false;
      }

      protected void Update() {
        if (!_IsTouching && !IsNearRestingState()) {
          if (_SnapBackInstantly) {
            SetToRest();
          }
          else {
            this.value = ((this.value - _RestingSliderValue) * _SliderValueDecayRate) + _RestingSliderValue;
          }
        }
      }

      private bool IsNearRestingState() {
        return ((this.normalizedValue <= _RestingSliderValue + float.Epsilon)
                && (this.normalizedValue >= _RestingSliderValue - float.Epsilon));
      }

      public override void OnPointerDown(UnityEngine.EventSystems.PointerEventData eventData) {
        base.OnPointerDown(eventData);
        _IsTouching = true;
      }

      public override void OnPointerUp(UnityEngine.EventSystems.PointerEventData eventData) {
        base.OnPointerUp(eventData);
        _IsTouching = false;
      }

      public void SetToRest() {
        this.value = _RestingSliderValue;
      }
    }
  }
}