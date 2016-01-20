using UnityEngine;
using System.Collections;
using Anki.UI;

namespace Simon {
  public class SimonGameSequencePanel : MonoBehaviour {

    [SerializeField]
    private AnkiTextLabel _SequenceLabel;

    public void SetSequenceText(int currentIndex, int sequenceCount) {
      string localized = Localization.Get(LocalizationKeys.kSimonGameLabelStepsLeft);
      localized = string.Format(Localization.GetCultureInfo(), localized, currentIndex, sequenceCount);
      _SequenceLabel.text = localized;
    }
  }
}
