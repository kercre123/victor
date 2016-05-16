using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SimpleFPSCounterLabel : MonoBehaviour {

  [SerializeField]
  private Text _FpsLabel;

  [SerializeField]
  private Text _FpsAvgLabel;

  [SerializeField]
  private Button _CloseButton;

  // Use this for initialization
  private void Start() {
    _CloseButton.onClick.AddListener(HandleCloseButtonClick);
  }

  private void OnDestroy() {
    _CloseButton.onClick.RemoveListener(HandleCloseButtonClick);
  }

  public void SetFPS(float newFPS) {
    _FpsLabel.text = newFPS.ToString();
  }

  public void SetAvgFPS(float newFPS) {
    _FpsAvgLabel.text = newFPS.ToString("F2");
  }

  private void HandleCloseButtonClick() {
    Destroy(gameObject);
  }
}
