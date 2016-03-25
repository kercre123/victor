using UnityEngine;
using System.Collections;

public class FactoryOptionsPanel : MonoBehaviour {

  public System.Action<int> OnSetTestNumber;
  public System.Action<int> OnSetStationNumber;

  [SerializeField]
  private UnityEngine.UI.Button _CloseButton;

  [SerializeField]
  public UnityEngine.UI.InputField _StationNumberInput;

  [SerializeField]
  public UnityEngine.UI.InputField _TestNumberInput;

  // Use this for initialization
  void Start() {
    _CloseButton.onClick.AddListener(() => GameObject.Destroy(gameObject));
    _StationNumberInput.onEndEdit.AddListener(HandleOnSetStationNumber);
    _TestNumberInput.onEndEdit.AddListener(HandleOnSetTestNumber);
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
}
