using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using Anki.Cozmo;

namespace Cozmo.Upgrades.GuardDog {
  public class GuardDogModal : BaseModal {

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
      LightCubeSprite upsidedownPrefab = CubePalette.Instance.UpsideDownLightCubeSpritePrefab;
      GameObject newCube = UIManager.CreateUIElement(upsidedownPrefab, lightCubeContainer);
      LightCubeSprite newCubeScript = newCube.GetComponent<LightCubeSprite>();
      newCubeScript.SetIcon(lightCubeType);
      newCubeScript.SetColor(CubePalette.Instance.ReadyColor.lightColor);
    }
  }
}