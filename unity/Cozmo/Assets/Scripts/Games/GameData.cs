using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class GameData : MonoBehaviour {
  public static GameData instance = null;

  public Dictionary<string, LevelData> levelData { get; private set; }

  [System.Serializable]
  public struct Game {
    public string name;
    public LevelData[] levels;
  }

  [System.Serializable]
  public struct LevelData {
    [HideInInspector] public string name;
    // for inspector info
    public int[] stars;
    public int scoreToWin;
    public float maxPlayTime;
  }

  public Game[] games;

  private void Awake() {
    if (instance != null) {
      GameObject.Destroy(gameObject);
      return;
    }
    
    instance = this;
    levelData = new Dictionary<string, LevelData>();
  }

  private void OnDestroy() {
    if (instance == this)
      instance = null;
  }

  private void OnEnable() {
    levelData.Clear();

    for (int i = 0; i < games.Length; ++i) {
      for (int j = 0; j < games[i].levels.Length; ++j) {
        levelData.Add(games[i].name + games[i].levels[j].name, games[i].levels[j]);
      }
    }
  }
}
