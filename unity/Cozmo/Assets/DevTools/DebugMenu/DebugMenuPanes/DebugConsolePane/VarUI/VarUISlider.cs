using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

public class VarUISlider : ConsoleVarLine {

  [SerializeField]
  private Slider _Slider;

  [SerializeField]
  private Text _SliderLabel;

  public override void Init(DebugConsoleData.DebugConsoleVarData singleVar) {
    base.Init(singleVar);

  }
    
}
