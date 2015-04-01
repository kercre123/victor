using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using Vectrosity;

public class ObservedObjectButton1 : ObservedObjectButton
{
	[SerializeField] protected Material lineMaterial;
	[SerializeField] protected float lineWidth;
	[SerializeField] protected Color lineColor;

	[System.NonSerialized] public ObservedObjectBox1 box;
	[System.NonSerialized] public VectorLine line;

	protected Vector3[] corners;
	public Vector3 position
	{
		get // top left corner
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

			return corners[1];
		}
	}

	protected void Awake()
	{
		line = new VectorLine( transform.name + "Line", new List<Vector2>(), lineMaterial, lineWidth );

		line.SetColor( lineColor );

		line.useViewportCoords = false;
	}

	public override void Selection()
	{
		base.Selection();
		
		box.audio.PlayOneShot( select );
	}
}
