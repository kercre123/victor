using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

namespace Anki.Debug {
  public class ConsoleCategoryPanel : MonoBehaviour {
    [SerializeField]
    public RectTransform _UIContainer;

    [SerializeField]
    public Text _TitleText;

    public string _CategoryName;
  }
}
