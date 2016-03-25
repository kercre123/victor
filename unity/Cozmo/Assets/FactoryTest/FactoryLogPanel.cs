using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class FactoryLogPanel : MonoBehaviour {
  [SerializeField]
  UnityEngine.UI.Text _LogTextField;

  [SerializeField]
  private UnityEngine.UI.ScrollRect _TextScrollRect;

  [SerializeField]
  private UnityEngine.UI.Button _CloseButton;

  private string _LogText;

  void Start() {
    _CloseButton.onClick.AddListener(ClosePanel);
  }

  public void UpdateLogText(List<string> logQueue) {
    _LogText = "";
    for (int i = logQueue.Count - 5; i < logQueue.Count; ++i) {
      _LogText += logQueue[i];
    }
    _LogTextField.text = _LogText;
    _TextScrollRect.verticalNormalizedPosition = 0;
  }

  private void ClosePanel() {
    GameObject.Destroy(gameObject);
  }
}
