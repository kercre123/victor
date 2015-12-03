using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

namespace DataPersistence {
  public class DataPersistencePane : MonoBehaviour {

    [SerializeField]
    private Button _ResetSaveDataButton;

    private void Start() {
      _ResetSaveDataButton.onClick.AddListener(HandleResetSaveDataButtonClicked);
    }

    private void OnDestroy() {
      _ResetSaveDataButton.onClick.RemoveListener(HandleResetSaveDataButtonClicked);
    }
      
    private void HandleResetSaveDataButtonClicked() {
      // use reflection to change readonly field
      typeof(DataPersistenceManager).GetField("Data").SetValue(DataPersistenceManager.Instance, new SaveData());
      DataPersistenceManager.Instance.Save();
    }
  }
}