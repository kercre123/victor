using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;

namespace Cutscenes {
  public class SpeechBubbleDialog : BaseDialog {

    private string _DialogID;

    [SerializeField]
    private Image _SpeakerImage;

    [SerializeField]
    private Text _Text;

    [SerializeField]
    private float _Duration;

    // Initializes this SpeechBubble with the specified line data
    public void Initialize(CutsceneData.CutsceneLine data) {
      _DialogID = data.lineID;
      _Text.text = data.text;
      // TODO: Set speaker image to the appropriate sprite
      // TODO: Play VO
    }


    protected override void ConstructCloseAnimation(Sequence closeAnimation) {
    }

    protected override void CleanUp() {
      // TODO: Cleanup logic
    }

  }
}
