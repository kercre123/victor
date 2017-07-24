using Cozmo.UI;
using System.Collections.Generic;
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

  public class VoiceLearnCommandsModal : BaseModal {
    [SerializeField]
    private SwipeSlides _SwipeSlidesScript;

    [SerializeField]
    private GameObject[] _SlidePrefabsBeforeCommands;

    [SerializeField]
    private UnlockableVoiceCommandSlide _VoiceCommandListSlidePrefab;

    [SerializeField]
    private GameObject[] _SlidePrefabsAfterCommands;

    [SerializeField]
    private VoiceCommandData[] _VoiceCommandData;

    private void Start() {
      List<VoiceCommandData> validVoiceCommands = GetValidVoiceCommands();
      int numCommandSlides = GetNumCommandSlides(validVoiceCommands.Count);
      List<GameObject> slidePrefabs = CompileSlidePrefabsToDisplay(numCommandSlides);
      _SwipeSlidesScript.Initialize(slidePrefabs.ToArray());

      // Get and init slides
      if (numCommandSlides > 0) {
        int firstCommandSlideIndex = _SlidePrefabsBeforeCommands.Length;
        int lastCommandSlideIndex = firstCommandSlideIndex + numCommandSlides;
        int commandIndex = 0;
        for (int slideIndex = firstCommandSlideIndex; slideIndex < lastCommandSlideIndex; slideIndex++) {
          GameObject slideInstance = _SwipeSlidesScript.GetSlideInstanceAt(slideIndex);
          UnlockableVoiceCommandSlide slideScript = slideInstance.GetComponent<UnlockableVoiceCommandSlide>();

          int numCommandsPerSlide = UnlockableVoiceCommandSlide.kMaxCellsPerSlide;
          int numCommandsLeft = (validVoiceCommands.Count - commandIndex);
          int numCommandsInSlide = (numCommandsLeft > numCommandsPerSlide) ? numCommandsPerSlide : numCommandsLeft;
          slideScript.Initialize(validVoiceCommands.GetRange(commandIndex, numCommandsInSlide).ToArray());
          commandIndex += numCommandsPerSlide;
        }
      }
    }

    private List<VoiceCommandData> GetValidVoiceCommands() {
      List<VoiceCommandData> validVoiceCommands = new List<VoiceCommandData>();
      for (int i = 0; i < _VoiceCommandData.Length; i++) {
        VoiceCommandData currentData = _VoiceCommandData[i];
        if (currentData.PrereqUnlockId == UnlockId.Invalid
          || UnlockablesManager.Instance.IsUnlocked(currentData.PrereqUnlockId)) {
          validVoiceCommands.Add(currentData);
        }
      }
      return validVoiceCommands;
    }

    private int GetNumCommandSlides(int numValidCommands) {
      int numCommandSlides = 0;
      if (numValidCommands > 0) {
        int numCommandsPerSlide = UnlockableVoiceCommandSlide.kMaxCellsPerSlide;
        numCommandSlides = (numValidCommands / numCommandsPerSlide);
        if (numValidCommands % numCommandsPerSlide > 0) {
          numCommandSlides++;
        }
      }
      return numCommandSlides;
    }

    private List<GameObject> CompileSlidePrefabsToDisplay(int numCommandSlides) {
      List<GameObject> slidePrefabs = new List<GameObject>();
      slidePrefabs.AddRange(_SlidePrefabsBeforeCommands);
      for (int i = 0; i < numCommandSlides; i++) {
        slidePrefabs.Add(_VoiceCommandListSlidePrefab.gameObject);
      }
      slidePrefabs.AddRange(_SlidePrefabsAfterCommands);
      return slidePrefabs;
    }
  }
}