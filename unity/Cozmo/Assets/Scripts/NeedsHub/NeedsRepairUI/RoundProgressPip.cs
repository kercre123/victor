using UnityEngine;

namespace Cozmo.Repair.UI {
  public class RoundProgressPip : MonoBehaviour {

    [SerializeField]
    private GameObject _RoundIncomplete = null;

    [SerializeField]
    private GameObject _RoundComplete = null;

    public void SetComplete(bool complete) {
      _RoundIncomplete.SetActive(!complete);
      _RoundComplete.SetActive(complete);
    }
  }
}
