using UnityEngine;
using System.Collections;

public class FactoryOptionsPanel : MonoBehaviour {

  public System.Action<int> OnSetTestNumber;
  public System.Action<int> OnSetStationNumber;
  public System.Action<bool> OnSetSim;

  [SerializeField]
  private UnityEngine.UI.Button _CloseButton;

  [SerializeField]
  public UnityEngine.UI.InputField _StationNumberInput;

  [SerializeField]
  public UnityEngine.UI.InputField _TestNumberInput;

  [SerializeField]
  private UnityEngine.UI.Toggle _SimToggle;

  [SerializeField]
  private UnityEngine.UI.Button _OTAButton;

  // Use this for initialization
  void Start() {
    _CloseButton.onClick.AddListener(() => GameObject.Destroy(gameObject));
    _StationNumberInput.onEndEdit.AddListener(HandleOnSetStationNumber);
    _TestNumberInput.onEndEdit.AddListener(HandleOnSetTestNumber);
    _SimToggle.onValueChanged.AddListener(HandleOnSetSimType);
    _OTAButton.onClick.AddListener(OnOTAButton);
  }

  void OnOTAButton() {
    
  }

  void HandleOnSetTestNumber(string input) {
    if (OnSetTestNumber != null) {
      OnSetTestNumber(int.Parse(input));
    }
  }

  void HandleOnSetStationNumber(string input) {
    if (OnSetStationNumber != null) {
      OnSetStationNumber(int.Parse(input));
    }
  }

  void HandleOnSetSimType(bool isSim) {
    if (OnSetSim != null) {
      OnSetSim(isSim);
    }
  }
}
