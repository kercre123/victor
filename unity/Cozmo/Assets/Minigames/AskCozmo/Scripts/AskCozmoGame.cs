using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace AskCozmo {

  public class AskCozmoGame : GameBase {

    private bool _AnimationPlaying = false;

    [SerializeField]
    private AskCozmoPanel _GamePanelPrefab;

    private AskCozmoPanel _GamePanel;

    private AskCozmoConfig _Config;

    void Start() {
      _GamePanel = UIManager.OpenDialog(_GamePanelPrefab).GetComponent<AskCozmoPanel>();
      _GamePanel.OnAskButtonPressed += OnAnswerRequested;
      CreateDefaultQuitButton();
    }

    public override void LoadMinigameConfig(MinigameConfigBase minigameConfig) {
      
      var askCozmoConfig = minigameConfig as AskCozmoConfig;

      if (askCozmoConfig == null) {
        DAS.Error(this, "Failed to load config as AskCozmoConfig!");
        // TODO: Handle this error
        return;
      }
      _Config = askCozmoConfig;

      DAS.Debug(this, _Config.Param1);
      DAS.Debug(this, _Config.Param2);

      for (int i = 0; i < _Config.ParamList.Length; ++i) {
        DAS.Debug(this, _Config.ParamList[i].ToString());
      }
    }

    public override void CleanUp() {
      if (_GamePanel != null) {
        UIManager.CloseDialogImmediately(_GamePanel);
      }
      DestroyDefaultQuitButton();
    }

    // user just asked the question and pressed the
    // give me answer button.
    public void OnAnswerRequested() {
      if (_AnimationPlaying) {
        return;
      }

      _AnimationPlaying = true;
      if (UnityEngine.Random.Range(0.0f, 1.0f) < 0.5f) {
        CurrentRobot.SendAnimation(AnimationName.kMajorWin, HandleAnimationDone);
      }
      else {
        CurrentRobot.SendAnimation(AnimationName.kShocked, HandleAnimationDone);
      }
    }

    void HandleAnimationDone(bool success) {
      _AnimationPlaying = false;
    }
  }

}
