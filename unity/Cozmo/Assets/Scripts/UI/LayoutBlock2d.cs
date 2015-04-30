using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class LayoutBlock2d : MonoBehaviour {

	[SerializeField] Image image_Block;
	[SerializeField] Image image_Symbol;
	[SerializeField] Image[] images_LED;
	[SerializeField] Image image_check;

	public BuildInstructionsCube Block { get; private set; }
	public int Dupe { get; private set; }


	public void Initialize(BuildInstructionsCube block, int dupe) {
		Block = block;
		Dupe = dupe;

		image_Block.color = block.baseColor;

		if(block.objectFamily == 3) {
			image_Symbol.sprite = CozmoPalette.instance.GetDigitSprite(Random.Range(1,6));
			foreach(Image led in images_LED) {
				led.color = CozmoPalette.instance.GetColorForActiveBlockType(block.activeBlockType);
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
	}

	public void Validate(bool valid=true) {
		image_check.gameObject.SetActive(valid);
	}

}
