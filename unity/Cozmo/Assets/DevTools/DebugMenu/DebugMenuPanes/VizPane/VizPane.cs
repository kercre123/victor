using UnityEngine;
using System.Collections;
using Anki.Cozmo;
using UnityEngine.UI;

namespace Anki.Cozmo.Viz {
  public class VizPane : MonoBehaviour {

    [SerializeField]
    private Text _InfoLabel;

    
    // Update is called once per frame
    private void Update () {

      _InfoLabel.text = string.Format("Current Behavior: {0}\n" +
        "Recent Mood Events: {1}\n",
        VizManager.Instance.Behavior,
        VizManager.Instance.RecentMoodEvents != null ? string.Join(", ", VizManager.Instance.RecentMoodEvents) : "");

    }
  }
}