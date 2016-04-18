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

  void Start() {
    _GreenPoints.text = DataPersistenceManager.Instance.Data.DefaultProfile.GreenPoints.ToString();
    _GreenLadderLevel.text = DataPersistenceManager.Instance.CurrentSession.GreenPointsLadderLevel.ToString();
    _Treats.text = DataPersistenceManager.Instance.Data.DefaultProfile.TreatCount.ToString();

    _GreenPoints.contentType = UnityEngine.UI.InputField.ContentType.IntegerNumber;
    _GreenLadderLevel.contentType = UnityEngine.UI.InputField.ContentType.IntegerNumber;
    _Treats.contentType = UnityEngine.UI.InputField.ContentType.IntegerNumber;

    _GreenPoints.onValueChanged.AddListener(GreenPointsChanged);
    _GreenLadderLevel.onValueChanged.AddListener(GreenLadderLevelChanged);
    _Treats.onValueChanged.AddListener(TreatsChanged);


    // TODO: Add generic input field and int field for adding / removing / setting items
    // Plus short-cut buttons for treats and green points (+100, -100)
  }

  private void GreenPointsChanged(string newValue) {
    PlayerManager.Instance.SetGreenPoints(int.Parse(newValue));
  }

  private void GreenLadderLevelChanged(string newValue) {
    DataPersistenceManager.Instance.CurrentSession.GreenPointsLadderLevel = int.Parse(newValue);
  }

  private void TreatsChanged(string newValue) {
    DataPersistenceManager.Instance.Data.DefaultProfile.TreatCount = int.Parse(newValue);
  }
}
