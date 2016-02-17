using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System;

namespace Anki.Cozmo.Audio {
  public class MixerStrip : MonoBehaviour {

    [SerializeField]
    private Slider _Slider;

    [SerializeField]
    private Text _Label;

    private VolumeParameters.VolumeType _VolumeType;

    public VolumeParameters.VolumeType VolumeType {
      get { return _VolumeType; }
      set { 
        _Label.text = value.ToString(); 
        _VolumeType = value;
      }
    }

    public float Value {
      get { return _Slider.value; }
      set { _Slider.value = value; }
    }

    public event Action<VolumeParameters.VolumeType, float> OnValueChange;

    private void Awake() {
      _Slider.onValueChanged.AddListener(HandleValueChanged);
    }

    private void HandleValueChanged(float value) {
      if (OnValueChange != null) {
        OnValueChange(VolumeType, value);
      }
    }
  }
}
