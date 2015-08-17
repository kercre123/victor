using UnityEngine;
using UnityEngine.UI;
using System.Collections;

/// <summary>
/// this component manages a two dimensional UI representation of a cozmo game cube
///    can be initialized in several ways:
///      - from a BuildInstructionsCube within a game's layout instructions
///      - from an ObservedObject that cozmo knows about
///      - from an explicit CubeType
/// </summary>
public class LayoutBlock2d : MonoBehaviour {

  [SerializeField] AudioClip validatedSound;
  [SerializeField] Image image_Block;
  [SerializeField] Image image_Symbol;
  [SerializeField] Image[] images_LED;
  [SerializeField] Image image_check;

  public BuildInstructionsCube Block { get; private set; }
  public int Dupe { get; private set; }

  ObservedObject currentObject = null;

  public bool Validated { get; private set; }

  //we can use this class to display a layout block
  public void Initialize(BuildInstructionsCube block, int dupe) {
    Block = block;
    Dupe = dupe;

    image_Block.color = block.baseColor;

    image_Symbol.sprite = CozmoPalette.instance.GetSpriteForObjectType(block.cubeType);
    foreach(Image led in images_LED) {
      //led.color = CozmoPalette.instance.GetColorForactiveBlockMode(block.activeBlockMode);
      led.gameObject.SetActive(block.isActive);
    }

    image_check.gameObject.SetActive(false);
    Validated = false;
    currentObject = null;
  }

  //we can also use this class to display an observed object
  public void Initialize(ObservedObject observedObject) {
    //image_Block.color = block.baseColor;

    if(observedObject.isActive) {
      ActiveBlock activeBlock = observedObject as ActiveBlock;
      image_Symbol.sprite = CozmoPalette.instance.GetSpriteForObjectType(observedObject.cubeType);
      foreach(Image led in images_LED) {
        led.color = CozmoPalette.instance.GetColorForActiveBlockMode(activeBlock.mode);
        led.gameObject.SetActive(true);
      }
    }
    else {
      image_Symbol.sprite = CozmoPalette.instance.GetSpriteForObjectType(observedObject.cubeType);
      foreach(Image led in images_LED) {
        led.gameObject.SetActive(false);
      }
    }
    
    image_check.gameObject.SetActive(false);
    Validated = false;
    Block = null;
    currentObject = observedObject;
  }

  public void Initialize(CubeType cubeType) {

    image_Symbol.sprite = CozmoPalette.instance.GetSpriteForObjectType(cubeType);

    if(cubeType == CubeType.LIGHT_CUBE) {
      image_Block.color = Color.grey;
      for(int i=0;i<images_LED.Length;i++) {
        Image led = images_LED[i];
        led.color = CozmoPalette.instance.GetColorForActiveBlockMode(ActiveBlock.Mode.Off);
        led.gameObject.SetActive(true);
      }
    }
    else {
      image_Block.color = Color.black;
      for(int i=0;i<images_LED.Length;i++) {
        Image led = images_LED[i];
        led.gameObject.SetActive(false);
      }
    }
    
    image_check.gameObject.SetActive(false);
    Validated = false;
    Block = null;
    currentObject = null;
  }

  public void SetLights(Color color) {
    SetLights(color, color, color, color);
  }

  public void SetLights(Color color1, Color color2, Color color3, Color color4) {
    if(images_LED.Length > 0) images_LED[0].color = color1;
    if(images_LED.Length > 1) images_LED[1].color = color2;
    if(images_LED.Length > 2) images_LED[2].color = color3;
    if(images_LED.Length > 3) images_LED[3].color = color4;
  }

  public void Validate(bool valid=true) {
    if(valid && !Validated) {
      if(validatedSound != null) AudioManager.PlayOneShot(validatedSound);
    }
    image_check.gameObject.SetActive(valid);
    Validated = valid;
  }

}
