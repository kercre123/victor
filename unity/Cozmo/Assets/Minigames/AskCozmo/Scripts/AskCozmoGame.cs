using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace AskCozmo {

  public class AskCozmoGame : GameBase {

    private bool _AnimationPlaying = false;

    [SerializeField]
    private AskCozmoPanel _GamePanelPrefab;

    private AskCozmoPanel _GamePanel;

    private string _Param1;
    private string _Param2;
    private List<int> _ListParam = new List<int>();

    void Start() {
      _GamePanel = UIManager.OpenDialog(_GamePanelPrefab).GetComponent<AskCozmoPanel>();
      _GamePanel.OnAskButtonPressed += OnAnswerRequested;
      CreateDefaultQuitButton();
    }

    public override void ParseMinigameParams(string paramsJSON) {
      base.ParseMinigameParams(paramsJSON);
      JSONObject minigameParamsObject = new JSONObject(paramsJSON);
      _Param1 = minigameParamsObject.GetField("AskGameParam1").str;
      _Param2 = minigameParamsObject.GetField("AskGameParam2").str;

      DAS.Debug("AskCozmoGame", _Param1);
      DAS.Debug("AskCozmoGame", _Param2);

      JSONObject arrayParam1 = minigameParamsObject.GetField("AskGameArrayParam1");
      for (int i = 0; i < arrayParam1.list.Count; ++i) {
        _ListParam.Add((int)arrayParam1[i].i);
      }

      for (int i = 0; i < _ListParam.Count; ++i) {
        DAS.Debug("AskCozmoGame", _ListParam[i].ToString());
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
        CurrentRobot.SendAnimation("majorWin", HandleAnimationDone);
      }
      else {
        CurrentRobot.SendAnimation("shocked", HandleAnimationDone);
      }
    }

    void HandleAnimationDone(bool success) {
      _AnimationPlaying = false;
    }
  }

}
