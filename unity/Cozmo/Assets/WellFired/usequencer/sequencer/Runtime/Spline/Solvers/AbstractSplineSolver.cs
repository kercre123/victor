using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace WellFired
{
	public abstract class AbstractSplineSolver : ScriptableObject
	{
		public const int TOTAL_SUBDIVISIONS_PER_NODE = 5;

		[SerializeField]
		protected List<SplineKeyframe> nodes;
		public List<SplineKeyframe> Nodes 
		{ 
			get { return nodes; }
			set { nodes = value; }
		}
		
		[SerializeField]
		protected float PathLength
		{
			get;
			set;
		}

		protected Dictionary<float, float> segmentTimeForDistance;
		
		private void OnEnable() 
		{
			if(Nodes == null)
				Nodes = new List<SplineKeyframe>();
		}

		public virtual void Build()
		{
			var totalSudivisions = Nodes.Count * TOTAL_SUBDIVISIONS_PER_NODE;
			PathLength = 0.0f;
			float timePerSlice = 1.0f / totalSudivisions;

			segmentTimeForDistance = new Dictionary<float, float>(totalSudivisions);
			var lastPoint = GetPosition(0.0f);

	        for(var i = 1; i < totalSudivisions + 1; i++)
	        {
	            float currentTime = timePerSlice * i;
				var currentPoint = GetPosition(currentTime);
	            PathLength += Vector3.Distance(currentPoint, lastPoint);
	            lastPoint = currentPoint;
				segmentTimeForDistance.Add(currentTime, PathLength);
	        }
		}

		public virtual Vector3 GetPositionOnPath(float time)
		{
			var targetDistance = PathLength * time;
			var previousNodeTime = 0.0f;
			var previousNodeLength = 0.0f;
			var nextNodeTime = 0.0f;
			var nextNodeLength = 0.0f;

			foreach(var item in segmentTimeForDistance)
			{
			    if(item.Value >= targetDistance)
			    {
			        nextNodeTime = item.Key;
			        nextNodeLength = item.Value;
					
			        if( previousNodeTime > 0 )
						previousNodeLength = segmentTimeForDistance[previousNodeTime];
	
			        break;
			    }
			    previousNodeTime = item.Key;
			}
			
			// translate the values from the lookup table estimating the arc length between our known nodes from the lookup table
			var segmentTime = nextNodeTime - previousNodeTime;
			var segmentLength = nextNodeLength - previousNodeLength;
			var distanceIntoSegment = targetDistance - previousNodeLength;
			
			time = previousNodeTime + (distanceIntoSegment / segmentLength) * segmentTime;
			return GetPosition(time);
		}

		public void Reverse()
		{
			Nodes.Reverse();
		}

		public void OnInternalDrawGizmos(Color splineColor, float displayResolution)
		{	
			Display(splineColor);

			var currentDisplayResolution = displayResolution;
			using(new WellFired.Shared.GizmosChangeColor(splineColor))
			{
				var previousPosition = GetPosition(0.0f);
				currentDisplayResolution *= Nodes.Count;
				for(var i = 1.0f; i <= currentDisplayResolution; i += 1.0f)
				{
					var t = i / currentDisplayResolution;
					var currentPosition = GetPosition(t);
					Gizmos.DrawLine(currentPosition, previousPosition);
					previousPosition = currentPosition;
				}
			}
		}

		public abstract void Display(Color splineColor);
		public abstract void Close();
		public abstract Vector3 GetPosition(float time);
	}
}