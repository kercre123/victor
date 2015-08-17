using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;
using UnityEditor;
using System.Collections;

[CustomEditor(typeof(GameData))]
public class GameDataEditor : Editor 
{
  public override void OnInspectorGUI()
  {
    base.OnInspectorGUI();

    GameData gameData = target as GameData;

    for( int i = 0; i < gameData.games.Length; ++i )
    {
      for( int j = 0; j < gameData.games[i].levels.Length; ++j )
      {
        gameData.games[i].levels[j].name = (j+1).ToString();

        if( gameData.games[i].levels[j].stars.Length != GameController.STAR_COUNT ) gameData.games[i].levels[j].stars = new int[GameController.STAR_COUNT];
      }
    }
  }
}
