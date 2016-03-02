using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Cozmo.UI;

namespace Playhouse {
  public class PlayhousePanel : BaseView {

    [SerializeField]
    public Anki.UI.AnkiButton _StartPlayButton;

    [SerializeField]
    public UnityEngine.UI.Dropdown _Dropdown1;

    [SerializeField]
    public UnityEngine.UI.Dropdown _Dropdown2;

    [SerializeField]
    public UnityEngine.UI.Dropdown _Dropdown3;

    public void Initialize() {
      // populate dropdowns with animations.
      PopulateAnimations(_Dropdown1);
      PopulateAnimations(_Dropdown2);
      PopulateAnimations(_Dropdown3);
    }

    private void PopulateAnimations(UnityEngine.UI.Dropdown dropdown) {
      List<string> robotAnimationNames = RobotEngineManager.Instance.GetRobotAnimationNames();
      robotAnimationNames.Sort();
      for (int i = 0; i < robotAnimationNames.Count; ++i) {
        UnityEngine.UI.Dropdown.OptionData optionData = new UnityEngine.UI.Dropdown.OptionData();
        optionData.text = robotAnimationNames[i];
        dropdown.options.Add(optionData);
      }
      dropdown.value = 0;
    }

    public List<string> GetAnimationList() {
      List<string> animationList = new List<string>();
      animationList.Add(_Dropdown1.options[_Dropdown1.value]);
      animationList.Add(_Dropdown2.options[_Dropdown2.value]);
      animationList.Add(_Dropdown3.options[_Dropdown3.value]);
      return animationList;
    }

    protected override void CleanUp() {

    }
  }

}
