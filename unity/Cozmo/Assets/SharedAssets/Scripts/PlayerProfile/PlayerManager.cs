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

  public delegate void GreenPointsUpdateHandler(int points, int maxPoints);

  public delegate void ChestGainedHandler();

  public GreenPointsUpdateHandler GreenPointsUpdate;

  public ChestGainedHandler ChestGained;

  public int GetGreenPointsLadderMax() {
    //int ladderLevel = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.LastOrDefault().GreenPointsLadderLevel;
    return 100;
  }

  public void AddGreenPoints(int points) {
    DataPersistenceManager.Instance.Data.DefaultProfile.GreenPoints += points;
    if (GreenPointsUpdate != null) {
      GreenPointsUpdate(DataPersistenceManager.Instance.Data.DefaultProfile.GreenPoints, GetGreenPointsLadderMax());
    }
  }
}
