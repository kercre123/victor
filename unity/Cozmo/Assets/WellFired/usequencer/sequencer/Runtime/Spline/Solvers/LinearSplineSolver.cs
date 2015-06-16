using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace WellFired
{
	[System.Serializable]
	public class LinearSplineSolver : AbstractSplineSolver
	{
		private Dictionary<int, float> segmentStartLocations;
		private Dictionary<int, float> segmentDistances;
		private int currentSegment;
		
		public override void Build()
		{
			segmentStartLocations = new Dictionary<int, float>(Nodes.Count - 2);
			segmentDistances = new Dictionary<int, float>(Nodes.Count - 1);
	
			for( var i = 0; i < Nodes.Count - 1; i++ )
			{
				// calculate the distance to the next node
				var distance = Vector3.Distance(Nodes[i].Position, Nodes[i + 1].Position);
				segmentDistances.Add(i, distance);
				PathLength += distance;
			}

			// now that we have the total length we can loop back through and calculate the segmentStartLocations
			var accruedRouteLength = 0f;
			for(var i = 0; i < segmentDistances.Count - 1; i++)
			{
				accruedRouteLength += segmentDistances[i];
				segmentStartLocations.Add(i + 1, accruedRouteLength / PathLength);
			}
		}
		
		
		public override void Close()
		{
			// add a node to close the route if necessary
			if(Nodes[0].Position != Nodes[Nodes.Count - 1].Position)
				Nodes.Add(Nodes[0]);
		}
		
		
		public override Vector3 GetPosition(float time)
		{
			return GetPositionOnPath(time);
		}
		
		
		public override Vector3 GetPositionOnPath(float time)
		{
			if(Nodes.Count < 3)
				return Vector3.Lerp(Nodes[0].Position, Nodes[1].Position, time);

			// which segment are we on?
			currentSegment = 0;
			foreach(var info in segmentStartLocations)
			{
				if(info.Value < time)
				{
					currentSegment = info.Key;
					continue;
				}
				
				break;
			}

			var totalDistanceTravelled = time * PathLength;
			var i = currentSegment - 1;
			while(i >= 0)
			{
				totalDistanceTravelled -= segmentDistances[i];
				--i;
			}
			
			return Vector3.Lerp(Nodes[currentSegment].Position, Nodes[currentSegment + 1].Position, totalDistanceTravelled / segmentDistances[currentSegment]);
		}
		
		public override void Display(Color splineColor)
		{

		}
	}
}