using UnityEngine;
using System.Collections;

public class FriendshipLevelConfig : ScriptableObject {

  [System.Serializable] 
  public struct FriendshipLevelData {
    public int PointsRequired;
    public string FriendshipLevelName;
  }

  [SerializeField]
  public FriendshipLevelData[] FriendshipLevels;
}
