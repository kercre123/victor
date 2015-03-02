using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SelectionButton : MonoBehaviour
{
	public Image image;
	public Text text;

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
}
