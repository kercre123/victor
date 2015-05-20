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

		if(block.isActive) {
			image_Symbol.sprite = CozmoPalette.instance.GetDigitSprite(Random.Range(1,6));
			foreach(Image led in images_LED) {
				//led.color = CozmoPalette.instance.GetColorForactiveBlockMode(block.activeBlockMode);
				led.gameObject.SetActive(true);
			}
		}
		else {
			image_Symbol.sprite = CozmoPalette.instance.GetSpriteForObjectType(block.objectType);
			foreach(Image led in images_LED) {
				led.gameObject.SetActive(false);
			}
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
			image_Symbol.sprite = CozmoPalette.instance.GetDigitSprite(spriteIndex);
			foreach(Image led in images_LED) {
				led.color = CozmoPalette.instance.GetColorForActiveBlockMode(activeBlock.mode);
				led.gameObject.SetActive(true);
			}
		}
		else {
			image_Symbol.sprite = CozmoPalette.instance.GetSpriteForObjectType((int)observedObject.ObjectType);
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
			if(audio != null && validatedSound != null) audio.PlayOneShot(validatedSound);
		}
		image_check.gameObject.SetActive(valid);
		validated = valid;
	}

}
