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
			Vector3 center = Vector3.zero;
			center.z = transform.position.z;
			
			Vector3[] corners = new Vector3[4];
			image.rectTransform.GetWorldCorners( corners );
			if( corners == null || corners.Length == 0 )
			{
				return center;
			}
			center = ( corners[0] + corners[2] ) * 0.5f;

			return center;
		}
	}

	public void Selection()
	{
		RobotEngineManager.instance.current.selectedObject = selectionBox.ID;
	}
}
