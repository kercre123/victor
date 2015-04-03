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
		get
		{
			if( corners == null )
			{
				corners = new Vector3[4];
			}
			
			rectTransform.GetWorldCorners( corners );

			return (corners[0] + corners[2]) * 0.5f;

//			if( corners.Length > 3 )
//			{
//				center = corners[0];
//				center.y = ( corners[0].y + corners[1].y ) * 0.5f;
//			}
//			
//			return center;
		}
	}

	protected override void Awake()
	{
		base.Awake();

		line = new VectorLine( transform.name + "Line", new List<Vector2>(), lineMaterial, lineWidth );
		line.SetColor( lineColor );
		line.useViewportCoords = false;
	}

	public override void Selection()
	{
		base.Selection();
		
		box.audio.PlayOneShot( select );
	}

	public override void SetColor(Color color) {
		base.SetColor(color);
		SetLineColor(color);
	}

	public void SetLineColor(Color color) {
		line.SetColor(color);
	}
}
