using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;
using Anki.UI;

namespace Conversations {
  public class SpeechBubble : BaseView {

    private string _LineID;

    [SerializeField]
    private Image _SpeakerImage;

    [SerializeField]
    private AnkiTextLabel _Text;

    // Initializes this SpeechBubble with the specified line data
    public void Initialize(ConversationLine line) {
      _LineID = line.LineKey;
      _Text.text = Localization.Get(_LineID);
    }

    protected override void ConstructCloseAnimation(Sequence closeAnimation) {
    }

    protected override void CleanUp() {
      
    }

  }
}
