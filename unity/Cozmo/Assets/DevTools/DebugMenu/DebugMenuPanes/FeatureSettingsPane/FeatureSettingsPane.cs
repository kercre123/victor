using UnityEngine;
using System.Collections;
using DataPersistence;

public class FeatureSettingsPane : MonoBehaviour {
  [SerializeField]
  UnityEngine.UI.Toggle _PressDemoToggle;

  private void Start() {
    _PressDemoToggle.isOn = DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.RunPressDemo;
    _PressDemoToggle.onValueChanged.AddListener(HandlePressDemoToggleValueChange);
  }

  private void HandlePressDemoToggleValueChange(bool val) {
    DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.RunPressDemo = val;
    DataPersistence.DataPersistenceManager.Instance.Save();
  }

}
