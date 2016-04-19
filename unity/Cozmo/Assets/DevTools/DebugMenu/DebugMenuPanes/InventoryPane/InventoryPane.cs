using UnityEngine;
using System.Collections;
using DataPersistence;

public class InventoryPane : MonoBehaviour {

  [SerializeField]
  UnityEngine.UI.InputField _GreenPoints;

  [SerializeField]
  UnityEngine.UI.InputField _GreenLadderLevel;

  [SerializeField]
  UnityEngine.UI.InputField _Treats;

  [SerializeField]
  UnityEngine.UI.InputField _HexPoints;

  // TODO: Allow add / remove any item
  // TODO: Add shortcut buttons for common items (treats, experience)
  void Start() {
    // _GreenPoints.text = DataPersistenceManager.Instance.Data.DefaultProfile.GreenPoints.ToString();
    _GreenPoints.contentType = UnityEngine.UI.InputField.ContentType.IntegerNumber;
    _GreenPoints.onValueChanged.AddListener(GreenPointsChanged);

    _GreenLadderLevel.text = DataPersistenceManager.Instance.CurrentSession.ChestsGained.ToString();
    _GreenLadderLevel.contentType = UnityEngine.UI.InputField.ContentType.IntegerNumber;
    _GreenLadderLevel.onValueChanged.AddListener(GreenLadderLevelChanged);

    // _Treats.text = DataPersistenceManager.Instance.Data.DefaultProfile.TreatCount.ToString();
    _Treats.contentType = UnityEngine.UI.InputField.ContentType.IntegerNumber;
    _Treats.onValueChanged.AddListener(TreatsChanged);

    _HexPoints.text = DataPersistenceManager.Instance.Data.DefaultProfile.HexPieces.ToString();
    _HexPoints.contentType = UnityEngine.UI.InputField.ContentType.IntegerNumber;
    _HexPoints.onValueChanged.AddListener(HexPointsChanged);
  }

  private void HexPointsChanged(string newValue) {
    DataPersistenceManager.Instance.Data.DefaultProfile.HexPieces = int.Parse(newValue);
    DataPersistenceManager.Instance.Save();
  }

  private void GreenPointsChanged(string newValue) {
    // PlayerManager.Instance.SetGreenPoints(int.Parse(newValue));
    DataPersistenceManager.Instance.Save();
  }

  private void GreenLadderLevelChanged(string newValue) {
    DataPersistenceManager.Instance.CurrentSession.ChestsGained = int.Parse(newValue);
    DataPersistenceManager.Instance.Save();
  }

  private void TreatsChanged(string newValue) {
    // DataPersistenceManager.Instance.Data.DefaultProfile.TreatCount = int.Parse(newValue);
    DataPersistenceManager.Instance.Save();
  }
}
