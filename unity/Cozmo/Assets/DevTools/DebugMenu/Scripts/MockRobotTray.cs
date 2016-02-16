using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class MockRobotTray : MonoBehaviour {

  [SerializeField]
  private DebugCube _DebugCubePrefab;

  [SerializeField]
  private RectTransform _CubeTray;

  private readonly Dictionary<int, DebugCube> _SpawnedCubes = new Dictionary<int, DebugCube>();

  public void Initialize(MockRobot robot) {

    var cubes = new[] {
      new LightCube(1, Anki.Cozmo.ObjectFamily.LightCube, Anki.Cozmo.ObjectType.Block_LIGHTCUBE1),
      new LightCube(2, Anki.Cozmo.ObjectFamily.LightCube, Anki.Cozmo.ObjectType.Block_LIGHTCUBE2),
      new LightCube(3, Anki.Cozmo.ObjectFamily.LightCube, Anki.Cozmo.ObjectType.Block_LIGHTCUBE3)
    };

    foreach (var cube in cubes) {
      var debugCube = UIManager.CreateUIElement(_DebugCubePrefab, _CubeTray).GetComponent<DebugCube>();
      debugCube.Initialize(cube);

      _SpawnedCubes[cube.ID] = debugCube;
      robot.LightCubes[cube.ID] = cube;
      robot.SeenObjects.Add(cube);
      robot.VisibleObjects.Add(cube);

      cube.MakeActiveAndVisible(true, true);
    }
  }
}
