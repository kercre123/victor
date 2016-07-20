using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Simon {
  public class SimonLivesWidget : MonoBehaviour {

    [SerializeField]
    private Image _PortraitImage;

    [SerializeField]
    private LayoutElement _TextContainer;

    [SerializeField]
    private AnkiTextLabel _SmallTextLabel;
    [SerializeField]
    private AnkiTextLabel _BigTextLabel;

    [SerializeField]
    private LayoutElement _LivesContainer;

    [SerializeField]
    private SegmentedBar _LivesCountBar;

    [SerializeField]
    private Image _Background;

    public void UpdateStatus(int lives, int maxLives, string headerLocKey, string statusLocKey) {
      _LivesCountBar.SetMaximumSegments(maxLives);
      _LivesCountBar.SetCurrentNumSegments(lives);
      _SmallTextLabel.text = headerLocKey == "" ? "" : Localization.Get(headerLocKey);
      _BigTextLabel.text = statusLocKey == "" ? "" : Localization.Get(statusLocKey);

    }
  }
}