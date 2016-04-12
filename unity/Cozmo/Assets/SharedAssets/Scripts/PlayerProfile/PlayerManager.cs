using UnityEngine;
using System.Collections;
using System.Linq;
using DataPersistence;

public class PlayerManager : MonoBehaviour {
  public static PlayerManager Instance { get; private set; }

  private void OnEnable() {
    if (Instance != null && Instance != this) {
      Destroy(gameObject);
      return;
    }
    else {
      Instance = this;
    }
  }

  [SerializeField]
  private ChestData _ChestData;

  public delegate void GreenPointsUpdateHandler(int points, int maxPoints);

  public delegate void ChestGainedHandler();

  public GreenPointsUpdateHandler GreenPointsUpdate;

  public ChestGainedHandler ChestGained;

  public int GetGreenPointsLadderMax() {
    if (_ChestData.GreenPointMaxLadders.Length == 0) {
      DAS.Error(this, "No green point ladders");
    }
    int ladderLevel = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault().GreenPointsLadderLevel;
    int ladderMaxValue = _ChestData.GreenPointMaxLadders.Last().Value; // default to max value
    for (int i = 1; i < _ChestData.GreenPointMaxLadders.Length; ++i) {
      if (ladderLevel > _ChestData.GreenPointMaxLadders[i].Level) {
        ladderMaxValue = _ChestData.GreenPointMaxLadders[i - 1].Level;
      }
    }
    return ladderMaxValue;
  }

  public void AddGreenPoints(int points) {
    DataPersistenceManager.Instance.Data.DefaultProfile.GreenPoints += points;
    if (GreenPointsUpdate != null) {
      GreenPointsUpdate(DataPersistenceManager.Instance.Data.DefaultProfile.GreenPoints, GetGreenPointsLadderMax());
    }
  }
}
