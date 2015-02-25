using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SelectionButton : MonoBehaviour
{
	[SerializeField] protected Image image;

	public Text text;
	public LineRenderer line;
	public CozmoVision.SelectionBox selectionBox { get; set; }

	public Vector3 position
	{
		get
		{
			Vector3 p = Vector3.zero;
			
			p.x = image.rectTransform.rect.center.x;
			p.y = image.rectTransform.rect.center.y;

			return p;
		}
	}

	public void Selection()
	{
		RobotEngineManager.instance.current.selectedObject = selectionBox.ID;
	}
}
