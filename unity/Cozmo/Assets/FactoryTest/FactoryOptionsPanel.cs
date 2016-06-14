using UnityEngine;
using System.Collections;

public class FactoryOptionsPanel : MonoBehaviour {

  public System.Action<int> OnSetTestNumber;
  public System.Action<int> OnSetStationNumber;
  public System.Action<bool> OnSetSim;
  public System.Action OnOTAButton;


  [SerializeField]
  private UnityEngine.UI.Button _CloseButton;

  [SerializeField]
  private UnityEngine.UI.Toggle _SimToggle;

  [SerializeField]
  private UnityEngine.UI.Button _OTAButton;

  public void Initialize(bool sim) {
    _SimToggle.isOn = sim;
  }

  // Use this for initialization
  void Start() {
    _CloseButton.onClick.AddListener(() => GameObject.Destroy(gameObject));
    _SimToggle.onValueChanged.AddListener(HandleOnSetSimType);
    _OTAButton.onClick.AddListener(HandleOTAButton);
  }

  void HandleOTAButton() {
    if (OnOTAButton != null) {
      OnOTAButton();
    }
  }

  void HandleOnSetSimType(bool isSim) {
    if (OnSetSim != null) {
      OnSetSim(isSim);
    }
  }
}
