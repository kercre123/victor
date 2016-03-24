using UnityEngine;
using System.Collections;

public class FactoryLogPanel : MonoBehaviour {
  [SerializeField]
  UnityEngine.UI.Text _LogTextField;

  [SerializeField]
  private UnityEngine.UI.ScrollRect _TextScrollRect;

  [SerializeField]
  private UnityEngine.UI.Button _CloseButton;

  void Start() {
    string test = "sdfsdf";
    for (int i = 0; i < 100; ++i) {
      test += i + "wee\n";
    }
    _LogTextField.text = test;
    _TextScrollRect.verticalNormalizedPosition = 0;
    _CloseButton.onClick.AddListener(ClosePanel);
  }

  private void ClosePanel() {
    GameObject.Destroy(gameObject);
  }
}
