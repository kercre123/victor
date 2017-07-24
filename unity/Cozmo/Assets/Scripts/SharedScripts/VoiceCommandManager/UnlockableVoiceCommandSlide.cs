using UnityEngine;

namespace Anki.Cozmo.VoiceCommand {

  public class UnlockableVoiceCommandSlide : MonoBehaviour {
    private const int _kNumCellsInRow = 2;
    private const int _kNumRowsPerSlide = 2;
    public const int kMaxCellsPerSlide = _kNumCellsInRow * _kNumRowsPerSlide;

    [SerializeField]
    private VoiceCommandCell _VoiceCommandCellPrefab;

    [SerializeField]
    private Transform _TopRowContainer;

    [SerializeField]
    private Transform _BottomRowContainer;

    public void Initialize(VoiceCommandData[] voiceCommandData) {
      if (voiceCommandData.Length <= 0 || voiceCommandData.Length > kMaxCellsPerSlide) {
        DAS.Error("UnlockableVoiceCommandSlide.DataValidation",
                  string.Format("voiceCommandData must have from 1-{0} elements!", kMaxCellsPerSlide));
      }

      int numCellsCreated = 0;
      for (int i = 0; i < voiceCommandData.Length; i++) {
        VoiceCommandData currentData = voiceCommandData[i];
        if (currentData.PrereqUnlockId == UnlockId.Invalid
            || UnlockablesManager.Instance.IsUnlocked(currentData.PrereqUnlockId)) {
          Transform cellParent = (numCellsCreated < _kNumCellsInRow) ? _TopRowContainer : _BottomRowContainer;
          GameObject newCellObject = UIManager.CreateUIElement(_VoiceCommandCellPrefab, cellParent);
          VoiceCommandCell newCell = newCellObject.GetComponent<VoiceCommandCell>();
          newCell.SetText(Localization.Get(currentData.CommandLocKey),
                          Localization.Get(currentData.DescriptionLocKey));
          numCellsCreated++;
        }
      }
    }
  }
}