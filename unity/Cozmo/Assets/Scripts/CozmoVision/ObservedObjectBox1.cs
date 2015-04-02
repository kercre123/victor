using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ObservedObjectBox1 : ObservedObjectButton
{
	[System.NonSerialized] public ObservedObjectButton1 button;

	protected float distance;
	protected float d;
	protected Vector3 position;
	protected Vector3[] corners;

	public Vector3 Position( Vector3 buttonPosition )
	{
		if( corners == null )
		{
			corners = new Vector3[4];
		}
		
		image.rectTransform.GetWorldCorners( corners );

		return (corners[2] + corners[3]) * 0.5f;


//		distance = float.MaxValue;
//		position = Vector3.zero;
//		position.z = image.transform.position.z;
//
//		for( int i = 0; i < corners.Length; ++i )
//		{
//			float d = Vector2.Distance( corners[i], buttonPosition );
//
//			if( d < distance )
//			{
//				position = corners[i];
//				distance = d;
//			}
//		}
//
//		return position;
	}
}
