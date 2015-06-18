using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace WellFired
{
	[System.Serializable]
	public class CubicBezierSplineSolver : AbstractSplineSolver
	{
		public CubicBezierSplineSolver( List<SplineKeyframe> nodes )
		{
			Nodes = nodes;
		}
		
		public override void Close()
		{
			
		}
		
		
		public override Vector3 GetPosition(float time)
		{
			float d = 1f - time;
			return d * d * d * Nodes[0].Position + 3f * d * d * time * Nodes[2].Position + 3f * d * time * time * Nodes[3].Position + time * time * time * Nodes[1].Position;
		}
	
		
		public override void Display(Color splineColor)
		{
			using(new WellFired.Shared.GizmosChangeColor(Color.red))
			{
				Gizmos.DrawLine(Nodes[0].Position, Nodes[2].Position);
				Gizmos.DrawLine(Nodes[3].Position, Nodes[1].Position);
			}
		}
	}
}