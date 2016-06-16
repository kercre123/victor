using UnityEngine;
using System.Collections;
using DataPersistence;

public class PlaytestSettingsPane : MonoBehaviour {
  [SerializeField]
  UnityEngine.UI.Button _ResetPlaytestSession;

  private void Start() {
    _ResetPlaytestSession.onClick.AddListener(ResetSession);
  }

  private void ResetSession() {
    RobotEngineManager.Instance.CurrentRobot.EraseAllEnrolledFaces();
    SkillSystem.Instance.DebugEraseStorage();
    // use reflection to change readonly field
    typeof(DataPersistenceManager).GetField("Data").SetValue(DataPersistenceManager.Instance, new SaveData());
    DataPersistenceManager.Instance.Save();
    Application.Quit();
  }

}
