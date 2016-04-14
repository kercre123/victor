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

  public delegate void ChestGainedHandler(int treatsGained);

  public GreenPointsUpdateHandler GreenPointsUpdate;

  public ChestGainedHandler ChestGained;

  public int GetGreenPointsLadderMax() {
    if (_ChestData.GreenPointMaxLadders.Length == 0) {
      DAS.Error(this, "No green point ladders");
    }
    int ladderLevel = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault().GreenPointsLadderLevel;
    int ladderMaxValue = _ChestData.GreenPointMaxLadders.Last().Value; // default to max value
    for (int i = 1; i < _ChestData.GreenPointMaxLadders.Length; ++i) {
      if (ladderLevel < _ChestData.GreenPointMaxLadders[i].Level) {
        ladderMaxValue = _ChestData.GreenPointMaxLadders[i - 1].Value;
        break;
      }
    }
    return ladderMaxValue;
  }

  public int GetTreatCount() {
    int ladderLevel = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault().GreenPointsLadderLevel;
    int treatValue = _ChestData.TreatRewardLadders.Last().Value; // default to max value
    for (int i = 1; i < _ChestData.TreatRewardLadders.Length; ++i) {
      if (ladderLevel > _ChestData.TreatRewardLadders[i].Level) {
        treatValue = _ChestData.TreatRewardLadders[i - 1].Level;
      }
    }
    return treatValue;
  }

  public void AddGreenPoints(int points) {
    int greenPoints = DataPersistenceManager.Instance.Data.DefaultProfile.GreenPoints += points;
    int currentLadderMax = GetGreenPointsLadderMax();
    while (greenPoints >= currentLadderMax) {
      int treatsGained = GetTreatCount();

      DAS.Info(this, "Chest unlocked! Gained " + treatsGained + " treats");

      DataPersistenceManager.Instance.Data.DefaultProfile.TreatCount += treatsGained;
      DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault().GreenPointsLadderLevel++;

      if (ChestGained != null) {
        ChestGained(treatsGained);
      }

      greenPoints -= currentLadderMax;
      currentLadderMax = GetGreenPointsLadderMax();
    }

    DataPersistenceManager.Instance.Data.DefaultProfile.GreenPoints = greenPoints;

    if (GreenPointsUpdate != null) {
      GreenPointsUpdate(greenPoints, GetGreenPointsLadderMax());
    }
  }
}
