using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using Anki.Cozmo;

namespace Cozmo.Upgrades {
  public class PyramidCubeUprightModal : BaseModal {

    [SerializeField]
    private Transform _LightCube1Container;

    [SerializeField]
    private Transform _LightCube2Container;

    [SerializeField]
    private Transform _LightCube3Container;

    private void Start() {
      CreateLightCube(_LightCube1Container, ObjectType.Block_LIGHTCUBE1);
      CreateLightCube(_LightCube2Container, ObjectType.Block_LIGHTCUBE2);
      CreateLightCube(_LightCube3Container, ObjectType.Block_LIGHTCUBE3);
    }

    private void CreateLightCube(Transform lightCubeContainer, ObjectType lightCubeType) {
      LightCubeSprite isometricPrefab = CubePalette.Instance.IsometricLightCubeSpritePrefab;
      GameObject newCube = UIManager.CreateUIElement(isometricPrefab, lightCubeContainer);
      LightCubeSprite newCubeScript = newCube.GetComponent<LightCubeSprite>();
      newCubeScript.SetIcon(lightCubeType);
      newCubeScript.SetColor(CubePalette.Instance.CubeUprightColor.lightColor);
    }
  }
}