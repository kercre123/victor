using UnityEngine;
using System;
using System.Collections;
using UnityEngine.UI;
using System.Collections.Generic;

public class AnimationPane : MonoBehaviour {

  [SerializeField]
  private InputField _SearchFieldButton;

  [SerializeField]
  private Button _AnimationButtonPrefab;

  [SerializeField]
  private RectTransform _ScrollViewContent;

  private class SearchableButton {
    public SearchableButton(Button inButton, string inText) {
      _Button = inButton;
      _ButtonText = inText;
    }

    public void UpdateActive(string searchString) {
      _Button.gameObject.SetActive((searchString.Length == 0) ||
                                   _ButtonText.ToLower().Contains(searchString.ToLower()));
    }

    private Button _Button;
    private string _ButtonText;
  }

  private List<SearchableButton> ButtonList = new List<SearchableButton>();

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
      animationButton.onClick.AddListener(() => {
        if (RobotEngineManager.Instance.CurrentRobot != null) {
          RobotEngineManager.Instance.CurrentRobot.SendAnimationTrigger(robotGroupNames[index]);
        }
      });
      animationButton.transform.FindChild("Text").GetComponent<Text>().text = robotGroupNames[i].ToString();
      animationButton.transform.SetParent(_ScrollViewContent, false);

      ButtonList.Add(new SearchableButton(animationButton, robotGroupNames[i].ToString()));
    }

    _SearchFieldButton.onValueChanged.AddListener(SearchValueChanged);
  }

  public void SearchValueChanged(string searchString) {
    foreach (SearchableButton button in ButtonList) {
      button.UpdateActive(searchString);
    }
  }
}
