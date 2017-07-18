using UnityEngine;

namespace Anki.Cozmo.VoiceCommand {
  [System.Serializable]
  public class VoiceCommandData {
    [SerializeField]
    private string _CommandLocKey;
    public string CommandLocKey { get { return _CommandLocKey; } }

    [SerializeField]
    private string _DescriptionLocKey;
    public string DescriptionLocKey { get { return _DescriptionLocKey; } }

    [SerializeField]
    private UnlockableInfo.SerializableUnlockIds _PrereqUnlockId;
    public UnlockId PrereqUnlockId { get { return _PrereqUnlockId.Value; } }
  }

  public class UnlockableVoiceCommandSlide : MonoBehaviour {
    private const int _kNumCellsInRow = 2;

    [SerializeField]
    private VoiceCommandCell _VoiceCommandCellPrefab;

    [SerializeField]
    private Transform _TopRowContainer;

    [SerializeField]
    private Transform _BottomRowContainer;

    [SerializeField]
    private VoiceCommandData[] _VoiceCommandData;

    private void Start() {
      if (_VoiceCommandData.Length <= 0 || _VoiceCommandData.Length > _kNumCellsInRow * 2) {
        DAS.Error("UnlockableVoiceCommandSlide.DataValidation",
                  string.Format("_VoiceCommandData must have from 1-{0} elements!", _kNumCellsInRow * 2));
      }

      int numCellsCreated = 0;
      for (int i = 0; i < _VoiceCommandData.Length; i++) {
        VoiceCommandData currentData = _VoiceCommandData[i];
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