using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.Cozmo;

public class ProgressionStatPane : MonoBehaviour {

  [SerializeField]
  private RectTransform _UIContainer;

  [SerializeField]
  private SingleStat _StatLinePrefab;

  private void Start() {
    // populate with known stats.
    Robot robot = RobotEngineManager.Instance.CurrentRobot;
    if (robot != null) {
      int statCount = (int)ProgressionStatType.Count;
      for (int i = 0; i < statCount; ++i) {
        GameObject statLine = UIManager.CreateUIElement(_StatLinePrefab.gameObject, _UIContainer);
        SingleStat statComp = statLine.GetComponent<SingleStat>();
        ProgressionStatType statEnum = (ProgressionStatType)i;
        statComp.Init(statEnum.ToString(), robot.ProgressionStats[i], statEnum);
      }
    }
  }
}
