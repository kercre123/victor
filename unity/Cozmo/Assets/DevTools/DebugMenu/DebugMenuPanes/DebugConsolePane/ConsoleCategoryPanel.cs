using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

namespace Anki.Debug {
  public class ConsoleCategoryPanel : MonoBehaviour {
    [SerializeField]
    public RectTransform UIContainer;

    [SerializeField]
    public Text TitleText;

    public string CategoryName;
  }
}
