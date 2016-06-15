using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.UI;
using Cozmo.UI;

namespace Simon {
  public class SimonTurnSlide : MonoBehaviour {

    [SerializeField]
    private Image _PortraitImage;

    [SerializeField]
    private AnkiTextLabel _StatusTextLabel;

    public void Initialize(Sprite portraitSprite, string statusLocKey) {
      _PortraitImage.sprite = portraitSprite;
      if (statusLocKey != null) {
        _StatusTextLabel.text = Localization.Get(statusLocKey);
      }
      else {
        _StatusTextLabel.text = "";
      }
    }
  }
}