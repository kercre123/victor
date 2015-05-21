using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class LayoutBlock2d : MonoBehaviour {

	[SerializeField] AudioClip validatedSound;
	[SerializeField] Image image_Block;
	[SerializeField] Image image_Symbol;
	[SerializeField] Image[] images_LED;
	[SerializeField] Image image_check;

	public BuildInstructionsCube Block { get; private set; }
	public int Dupe { get; private set; }

	ObservedObject currentObject = null;

	private bool validated = false;

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
		validated = false;
		currentObject = null;
	}

	int spriteIndex = 0;

	//we can also use this class to display an observed object
	public void Initialize(ObservedObject observedObject) {
		//image_Block.color = block.baseColor;
		if(observedObject != currentObject) {
			spriteIndex = Random.Range(1,6);
		}

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
		validated = false;
		Block = null;
		currentObject = observedObject;
	}

	public void Validate(bool valid=true) {
		if(valid && !validated) {
			if(validatedSound != null) AudioManager.PlayOneShot(validatedSound);
		}
		image_check.gameObject.SetActive(valid);
		validated = valid;
	}

}
