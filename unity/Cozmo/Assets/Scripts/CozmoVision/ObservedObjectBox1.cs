using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ObservedObjectBox1 : ObservedObjectButton
{
	protected Vector3[] corners;
	
	public Vector3 position
	{
		get
		{
			if( corners == null )
			{
				corners = new Vector3[4];
			}
			
			rectTransform.GetWorldCorners( corners );
			
			if( corners.Length > 3 )
			{
				return ( corners[2] + corners[3] ) * 0.5f;
			}

			Debug.LogWarning( "Did not find 4 corners for " + transform.name );
			return Vector3.zero;
		}
	}

	public bool IsInside( RectTransform reticle, Camera camera )
	{
		if( corners == null )
		{
			corners = new Vector3[4];
		}

		rectTransform.GetWorldCorners( corners );

		for( int i = 0; i < corners.Length; ++i )
		{
			if( !RectTransformUtility.RectangleContainsScreenPoint( reticle, corners[i], camera ) ) return false;
		}
		return true;
	}
}
