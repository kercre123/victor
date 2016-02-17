using UnityEngine;
using System.Collections;
using UnityEngine.UI;

public class AudioCombinedTab : MonoBehaviour {

  public string Text { 
    get { return _Label.text; }
    set { _Label.text = value; }
  }


  public event System.Action OnSelect;

  [SerializeField]
  private Text _Label;

  [SerializeField]
  private Toggle _Toggle;

  private void Awake() {
    _Toggle.onValueChanged.AddListener(HandleToggleToggled);      
  }

  private void HandleToggleToggled(bool val) {
    if (val && OnSelect != null) {
      OnSelect();
    }
  }

  public void SetToggleGroup(ToggleGroup toggleGroup) {
    _Toggle.group = toggleGroup;
  }

  public void Select() {
    _Toggle.isOn = true;
    // not sure if setting isOn will trigger the callback
    HandleToggleToggled(true);
  }

}
