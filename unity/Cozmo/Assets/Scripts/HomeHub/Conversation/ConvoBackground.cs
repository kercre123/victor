using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Conversations {
  // Class for handling the conversation background to hide everything behind speech bubbles mid conversation
  // Will add additional logic as it proves necessary.
  public class ConvoBackground : BaseModal {

    [SerializeField]
    private Image _Background;
    public AnkiButtonLegacy NextButton;

    public void InitButtonText(string locKey) {
      NextButton.Text = Localization.Get(locKey);
    }

    protected override void CleanUp() {

    }

  }
}
