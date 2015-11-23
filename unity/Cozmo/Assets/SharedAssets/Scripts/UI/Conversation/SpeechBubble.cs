using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;
using Anki.UI;

namespace Conversations {
  public class SpeechBubble : BaseView {

    private string _LineKey;

    [SerializeField]
    private Image _SpeakerImage;

    [SerializeField]
    private AnkiTextLabel _Text;

    // Initializes this SpeechBubble with the specified line data
    public void Initialize(ConversationLine line) {
      _LineKey = line.LineKey;
      _Text.text = Localization.Get(_LineKey);
    }

    protected override void ConstructCloseAnimation(Sequence closeAnimation) {
    }

    protected override void CleanUp() {
      
    }

  }
}
