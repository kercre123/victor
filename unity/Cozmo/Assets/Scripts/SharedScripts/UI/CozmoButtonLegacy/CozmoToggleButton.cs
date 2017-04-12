using UnityEngine;
using System.Collections;

namespace Cozmo.UI {
  public class CozmoToggleButton : CozmoButtonLegacy {
    public delegate void CozmoToggleChangeHandler(bool isNowOn);
    public event CozmoToggleChangeHandler OnValueChanged;

    [SerializeField]
    private bool _StartOn = true;

    [SerializeField]
    private GameObject _OnImageContainer;

    [SerializeField]
    private GameObject _OffImageContainer;

    private bool _CurrentlyOn;
    public bool IsCurrentlyOn {
      get { return _CurrentlyOn; }
    }

    protected override void Start() {
      base.Start();
      _CurrentlyOn = _StartOn;
      UpdateSelected();
    }

    private void UpdateSelected() {
      _OnImageContainer.SetActive(_CurrentlyOn);
      _OffImageContainer.SetActive(!_CurrentlyOn);

      if (OnValueChanged != null) {
        OnValueChanged(_CurrentlyOn);
      }
    }

    protected override void HandleOnPress() {
      base.HandleOnPress();
      _CurrentlyOn = !_CurrentlyOn;
      UpdateSelected();
    }
  }
}