using UnityEngine;
using System;
using System.Collections;
using UnityEngine.UI;
using System.Collections.Generic;

public class AnimationPane : MonoBehaviour {

  [SerializeField]
  private Button _AnimationButtonPrefab;

  [SerializeField]
  private RectTransform _ScrollViewContent;

  // Use this for initialization
  void Start() {
    List<Anki.Cozmo.AnimationTrigger> robotGroupNames = new List<Anki.Cozmo.AnimationTrigger>();
    for (int i = 0; i < (int)Anki.Cozmo.AnimationTrigger.Count; ++i) {
      Anki.Cozmo.AnimationTrigger ev = (Anki.Cozmo.AnimationTrigger)(i);
      robotGroupNames.Add(ev);
    }
    robotGroupNames.Sort();
    for (int i = 0; i < robotGroupNames.Count; ++i) {
      int index = i;
      Button animationButton = GameObject.Instantiate(_AnimationButtonPrefab.gameObject).GetComponent<Button>();
      animationButton.onClick.AddListener(() => RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(robotGroupNames[index]));
      animationButton.transform.FindChild("Text").GetComponent<Text>().text = robotGroupNames[i].ToString();
      animationButton.transform.SetParent(_ScrollViewContent, false);
    }

  }
}
