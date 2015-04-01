using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ObservedObjectBox1 : ObservedObjectButton
{
	[System.NonSerialized] public ObservedObjectButton1 button;

	protected Vector3[] corners;
	public Vector3 position
	{
		get // bottom right corner
		{
			/*Vector3 center = Vector3.zero;
			center.z = image.transform.position.z;*/
			
			if( corners == null )
			{
				corners = new Vector3[4];
			}
			
			image.rectTransform.GetWorldCorners( corners );
			
			if( corners.Length == 0 )
			{
				return Vector3.zero;
			}
			
			/*center = ( corners[0] + corners[2] ) * 0.5f;
			
			return center;*/

			return corners[3];
		}
	}
}
