using UnityEngine;
using UnityEngine.UI;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo.UI {
  public class WinnerStateSlide : MonoBehaviour {

    [SerializeField]
    private Image _PortraitImage;

    [SerializeField]
    private AnkiTextLabel _WinnerLabel;

    [SerializeField]
    private AnkiTextLabel _FooterLabel;

    public void Init(Sprite showImage, string winnerText, string footerText) {
      _PortraitImage.sprite = showImage;
      _WinnerLabel.text = winnerText;
      _FooterLabel.text = footerText;
    }
  }

}