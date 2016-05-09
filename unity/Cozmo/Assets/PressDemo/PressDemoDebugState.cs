using UnityEngine;
using System.Collections;

public class PressDemoDebugState : MonoBehaviour {

  [SerializeField]
  private UnityEngine.UI.Image _BaseImage;

  [SerializeField]
  private Sprite[] _Database;

  public void SetDebugImageIndex(int index) {
    _BaseImage.overrideSprite = _Database[index];
  }
}
