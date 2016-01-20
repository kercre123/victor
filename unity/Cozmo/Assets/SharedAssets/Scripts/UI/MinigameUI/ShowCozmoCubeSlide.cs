using UnityEngine;
using System.Collections;

public class ShowCozmoCubeSlide : MonoBehaviour {

  // INGO TODO: Using a text field for now and then adding
  // functionality to show images later
  [SerializeField]
  private Anki.UI.AnkiTextLabel _NumCubesText;
  private int _NumCubesToShow;

  public void Initialize(int numCubesToShow) {
    _NumCubesText.text = "0 / " + numCubesToShow;
    _NumCubesToShow = numCubesToShow;
  }

  public void LightUpCubes(int numberCubes) {
    _NumCubesText.text = numberCubes + " / " + _NumCubesToShow;
  }
}
