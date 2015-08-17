using UnityEngine;
using UnityEngine.UI;
using System;
using System.Text.RegularExpressions;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// this component manages a scrollable list of LevelSelectButtons that are associated with a specific minigame
/// </summary>
public class LevelSelectPanel : MonoBehaviour {

  [SerializeField] Text textTitle;
  [SerializeField] string gameName = "Unknown";
  [SerializeField] int numLevels = 10;
  [SerializeField] int unlockedLevels = 1;
  [SerializeField] GameObject levelSelectionPrefab;
  [SerializeField] ScrollRect scrollRect;
  [SerializeField] Sprite[] previews;
  [SerializeField] AudioClip clickSound;
   
  List<LevelSelectButton> levelSelectButtons = new List<LevelSelectButton>();

  private Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

  void Awake() {

    textTitle.text = gameName.ToUpper();

    float width = 0f;

    for (int i = 0; i < numLevels; i++) {
      GameObject buttonObj = (GameObject)GameObject.Instantiate(levelSelectionPrefab);

      RectTransform rectT = buttonObj.transform as RectTransform;

      rectT.SetParent(scrollRect.content, false);
      width += rectT.sizeDelta.x;

      LevelSelectButton selectButton = buttonObj.GetComponent<LevelSelectButton>();

      Sprite preview = null;
      if (previews != null && previews.Length > 0) {
        int spriteIndex = Mathf.Clamp(i, 0, previews.Length - 1);
        preview = previews[spriteIndex];
      }

      int level = i + 1;

      bool interactive = level <= unlockedLevels;

      selectButton.Initialize(level.ToString(), preview, interactive ? PlayerPrefs.GetInt(gameName + level + "_stars", 0) : 0, interactive, delegate {
        LaunchGame(level);
      });

      levelSelectButtons.Add(selectButton);
    }

    RectTransform scrollRectT = scrollRect.content.transform as RectTransform;
    Vector2 size = scrollRectT.sizeDelta;
    size.x = width;
    scrollRectT.sizeDelta = size;

    if (robot != null) {
      robot.ClearAllObjects();
      robot.ClearData();
    }
  }

  void LaunchGame(int level) {
    if (clickSound != null)
      AudioManager.PlayOneShot(clickSound);
    //Debug.Log("LevelSelectPanel LaunchGame gameName("+gameName+") level("+level+")");
    PlayerPrefs.SetString("CurrentGame", gameName);
    PlayerPrefs.SetInt(gameName + "_CurrentLevel", level);

    string sceneName = Regex.Replace(gameName, "[ ]", string.Empty);
    
    Application.LoadLevel(sceneName);

  }

}
