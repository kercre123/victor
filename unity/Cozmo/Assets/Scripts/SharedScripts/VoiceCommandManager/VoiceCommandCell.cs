using UnityEngine;

namespace Anki.Cozmo.VoiceCommand {
  public class VoiceCommandCell : MonoBehaviour {
    [SerializeField]
    private CozmoText _CommandLabel;

    [SerializeField]
    private CozmoText _DescriptionLabel;

    public void SetText(string commandLabelText,
                        string descriptionLabelText) {
      _CommandLabel.text = commandLabelText;
      _DescriptionLabel.text = descriptionLabelText;
    }
  }
}