using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Conversations {
  public class SpeechBubble : BaseModal {

    private string _LineKey;

    [SerializeField]
    private Image _SpeakerImage;

    [SerializeField]
    private AnkiTextLegacy _Text;

    // Initializes this SpeechBubble with the specified line data
    public void Initialize(ConversationLine line) {
      _LineKey = line.LineKey;
      _Text.text = Localization.Get(_LineKey);
    }

    protected override void CleanUp() {

    }

  }
}
