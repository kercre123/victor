using UnityEngine;
using System.Collections;

public class ShowCozmoCubeSlide : MonoBehaviour {

  [SerializeField]
  private Cozmo.UI.SegmentedBar _NumCubesBar;

  public void Initialize(int numCubesToShow) {
    _NumCubesBar.SetMaximumSegments(numCubesToShow);
  }

  public void LightUpCubes(int numberCubes) {
    _NumCubesBar.SetCurrentNumSegments(numberCubes);
  }
}
