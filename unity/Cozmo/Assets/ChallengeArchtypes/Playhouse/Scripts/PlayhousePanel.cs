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

    public List<string> GetAnimationList() {
      List<string> animationList = new List<string>();
      return animationList;
    }

    protected override void CleanUp() {

    }
  }

}
