
/**

The debug console supports types: Ranged floats, int/float textfields, bools, and function calls.
                                  It can be used for both private/public members and static vars

Adding a function example:
DebugConsoleData.Instance.AddConsoleFunction("Function Label", "Category Name",
                                                   (string param) => { DAS.Info("example","example"); });

Adding/Removing an instance member:
DebugConsoleData.Instance.AddConsoleVar("_VariableName", "Category Name", this);
DebugConsoleData.Instance.RemoveConsoleData("_VariableName", "Category Name");

Specifying a range:
DebugConsoleData.Instance.AddConsoleVar("_SliderTest", "Category Name", this, -10.0f, 10.0f);

Adding a static member ( can be removed too ):
DebugConsoleData.Instance.AddConsoleVar("_sExampleStaticVar", "Category.Name", typeof(ThisClassName));

*/
namespace Anki.Debug {
  public class ConsoleInitUnityData {

    // Things here that are too small or don't fit many other places.
    // However in general prefer to put these with the classes they control.
    public void Init() {
      DebugConsoleData.Instance.AddConsoleFunction("Erase Skills", "Save Data Persistance",
                                                              (string str) => { SkillSystem.Instance.DebugEraseStorage(); });
      DebugConsoleData.Instance.AddConsoleFunction("Erase All Enrolled Faces", "Save Data Persistance",
                                                        (string str) => { RobotEngineManager.Instance.CurrentRobot.EraseAllEnrolledFaces(); });
      DebugConsoleData.Instance.AddConsoleFunction("Reset All Robot Data", "Save Data Persistance", HandleResetRobot);
      DebugConsoleData.Instance.AddConsoleFunction("Reset PlayTest Session", "PlayTest", HandleResetSession);

      DebugConsoleData.Instance.AddConsoleVar("_CalibrationFocalX", "Camera Calibration", this);
      DebugConsoleData.Instance.AddConsoleVar("_CalibrationFocalY", "Camera Calibration", this);
      DebugConsoleData.Instance.AddConsoleVar("_CalibrationCenterX", "Camera Calibration", this);
      DebugConsoleData.Instance.AddConsoleVar("_CalibrationCenterY", "Camera Calibration", this);
      DebugConsoleData.Instance.AddConsoleFunction("Set Calibration", "Camera Calibration",
                                                              (string str) => { RobotEngineManager.Instance.CurrentRobot.SetCalibrationData(_CalibrationFocalX, _CalibrationFocalY, _CalibrationCenterX, _CalibrationCenterY); });
      DebugConsoleData.Instance.AddConsoleFunction("Upload Dev/DAS Logs", "Dev", (string str) => { CozmoBinding.cozmo_execute_background_transfers(); });
    }

    private float _CalibrationFocalX = 0;
    private float _CalibrationFocalY = 0;
    private float _CalibrationCenterX = 0;
    private float _CalibrationCenterY = 0;


    public void HandleResetRobot(string str) {
      if (RobotEngineManager.Instance != null && RobotEngineManager.Instance.CurrentRobot != null) {
        RobotEngineManager.Instance.CurrentRobot.EraseAllEnrolledFaces();
        SkillSystem.Instance.DebugEraseStorage();
        UnlockablesManager.Instance.TrySetUnlocked(Anki.Cozmo.UnlockId.Defaults, true);
      }
    }
    private void HandleResetSession(string str) {
      HandleResetRobot(str);
      // use reflection to change readonly field
      typeof(DataPersistence.DataPersistenceManager).GetField("Data").SetValue(DataPersistence.DataPersistenceManager.Instance, new DataPersistence.SaveData());
      DataPersistence.DataPersistenceManager.Instance.Save();
    }

  }
}
