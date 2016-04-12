using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class RobotSettingsPane : MonoBehaviour {

  [SerializeField]
  private Button _ToggleDebugStringButton;

  [SerializeField]
  private Toggle _ToggleDebugStringType;

  [SerializeField]
  private GameObject _RobotStateTextFieldPrefab;
  private GameObject _RobotStateTextFieldInstance;

  private void Start() {

    _ToggleDebugStringButton.onClick.AddListener(OnToggleDebugString);
    
    _ToggleDebugStringType.onValueChanged.AddListener(OnToggleDebugStringType);
  }

  private void OnToggleDebugStringType(bool check) {
    RobotStateTextField.UseAnimString(check);
  }

  private void OnToggleDebugString() {
    Canvas debugCanvas = DebugMenuManager.Instance.DebugOverlayCanvas;
    if (debugCanvas != null) {
      if (_RobotStateTextFieldInstance == null) {
        _RobotStateTextFieldInstance = UIManager.CreateUIElement(_RobotStateTextFieldPrefab, debugCanvas.transform);
      }
      else {
        Destroy(_RobotStateTextFieldInstance.gameObject);
      }
    }
    
  }

}
