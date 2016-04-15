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

  public delegate void ChestGainedHandler(int treatsGained, int hexGained);

  public GreenPointsUpdateHandler GreenPointsUpdate;

  public ChestGainedHandler ChestGained;

  public int GetGreenPointsLadderMax() {
    return GetCurrentLadderValue(_ChestData.GreenPointMaxLadder);
  }

  public int GetCurrentLadderValue(LadderLevel[] ladder) {
    int ladderLevel = 0;
    if (DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault() != null) {
      ladderLevel = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault().GreenPointsLadderLevel;
    }
    int value = ladder.Last().Value; // default to max value
    for (int i = 0; i < ladder.Length - 1; ++i) {
      if (ladderLevel >= ladder[i].Level && ladderLevel < ladder[i + 1].Level) {
        value = ladder[i].Value;
        break;
      }
    }
    return value;
  }

  public void SetGreenPoints(int points) {
    int greenPoints = points;
    int currentLadderMax = GetCurrentLadderValue(_ChestData.GreenPointMaxLadder);
    while (greenPoints >= currentLadderMax) {
      int treatsGained = GetCurrentLadderValue(_ChestData.TreatRewardLadder);
      int hexGained = GetCurrentLadderValue(_ChestData.HexRewardLadder);

      DAS.Info(this, "Chest unlocked! Gained " + treatsGained + " treats");

      DataPersistenceManager.Instance.Data.DefaultProfile.TreatCount += treatsGained;
      DataPersistenceManager.Instance.Data.DefaultProfile.HexPieces += hexGained;
      DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault().GreenPointsLadderLevel++;

      if (ChestGained != null) {
        ChestGained(treatsGained, hexGained);
      }

      greenPoints -= currentLadderMax;
      currentLadderMax = GetCurrentLadderValue(_ChestData.GreenPointMaxLadder);
    }

    DataPersistenceManager.Instance.Data.DefaultProfile.GreenPoints = greenPoints;

    if (GreenPointsUpdate != null) {
      GreenPointsUpdate(greenPoints, GetCurrentLadderValue(_ChestData.GreenPointMaxLadder));
    }
  }

  public void AddGreenPoints(int points) {
    SetGreenPoints(DataPersistenceManager.Instance.Data.DefaultProfile.GreenPoints + points);
  }
}
