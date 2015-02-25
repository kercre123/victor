using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SelectionButton : MonoBehaviour
{
	[SerializeField] protected Image image;

	public Text text;
	public LineRenderer line;
	public ImageTest.SelectionBox selectionBox { get; set; }

	public Vector3 position
	{
		get
		{
			Vector3 p = image.transform.position;
			
			p.x += image.rectTransform.rect.size.x * 0.5f;
			p.y -= image.rectTransform.rect.size.y * 0.5f;
			
			return p;
		}
	}

	public void Selection()
	{
		RobotEngineManager.instance.current.selectedObject = selectionBox.ID;
	}
}
