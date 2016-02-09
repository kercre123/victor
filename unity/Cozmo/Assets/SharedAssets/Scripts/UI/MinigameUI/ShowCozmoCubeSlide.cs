using UnityEngine;
using System.Collections;

public class ShowCozmoCubeSlide : MonoBehaviour {

  [SerializeField]
  private Cozmo.UI.SegmentedBar _NumCubesBar;

  [SerializeField]
  private Anki.UI.AnkiTextLabel _ShowCozmoCubesLabel;

  public void Initialize(int numCubesToShow) {
    _NumCubesBar.SetMaximumSegments(numCubesToShow);
    _ShowCozmoCubesLabel.text = Localization.GetWithArgs(LocalizationKeys.kMinigameLabelShowCubes,
      numCubesToShow);
  }

  public void LightUpCubes(int numberCubes) {
    _NumCubesBar.SetCurrentNumSegments(numberCubes);
  }
}
