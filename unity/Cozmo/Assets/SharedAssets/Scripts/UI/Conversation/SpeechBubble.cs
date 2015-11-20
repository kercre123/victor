using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;

namespace Conversations {
  public class SpeechBubble : BaseView {

    private string _DialogID;

    [SerializeField]
    private Image _SpeakerImage;

    [SerializeField]
    private Text _Text;

    // Initializes this SpeechBubble with the specified line data
    public void Initialize(ConversationData.ConversationLine data) {
      _DialogID = data.lineID;
      _Text.text = data.text;
      // TODO: Set speaker image to the appropriate sprite 
      // (speaker images need to fall within the same folder and naming convention.)
      // TODO: Play VO - Set to Advance to the next line when VO ends
      Debug.Log(string.Format("SpeechBubble.Initialize - {0}",_DialogID));
    }

    protected override void ConstructCloseAnimation(Sequence closeAnimation) {
    }

    protected override void CleanUp() {
      // TODO: Cleanup logic
      // TODO: Silence current VO assuming it isn't done
    }

  }
}
