using UnityEngine;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	[System.Serializable]
	public class Spline
	{
		public int CurrentSegment 
		{ 
			get; 
			private set; 
		}

		public bool IsClosed 
		{ 
			get; 
			private set; 
		}
		
		private bool IsReversed
		{ 
			get; 
			set; 
		}

		[SerializeField]
		private Color splineColor = Color.white;
		public Color SplineColor
		{
			get { return splineColor; }
			set { splineColor = value; }
		}
		
		[SerializeField]
		private float displayResolution = 20.0f;
		public float DisplayResolution
		{
			get { return displayResolution; }
			set { displayResolution = value; }
		}

		public List<SplineKeyframe> Nodes
		{
			get { return SplineSolver.Nodes; }
		}

		[SerializeField]
		private AbstractSplineSolver splineSolver;
		public AbstractSplineSolver SplineSolver
		{ 
			get { return splineSolver; }
			private set { splineSolver = value; }
		}
		
		public Vector3 LastNode
		{
			get { return SplineSolver.Nodes[SplineSolver.Nodes.Count].Position; }
		}

		public void BuildFromKeyframes(List<SplineKeyframe> keyframes)
		{	
			bool keyframeDifference = SplineSolver == null;

			if(SplineSolver != null)
			{
				keyframeDifference = keyframes.Count() != Nodes.Count();
				if(!(SplineSolver is LinearSplineSolver) && SplineSolver.Nodes.Count == 2)
					keyframeDifference = true;
				else if(!(SplineSolver is QuadraticSplineSolver) && SplineSolver.Nodes.Count == 3)
					keyframeDifference = true;
				else if(!(SplineSolver is QuadraticSplineSolver) && SplineSolver.Nodes.Count == 4)
					keyframeDifference = true;
				else if(!(SplineSolver is CatmullRomSplineSolver))
					keyframeDifference = true;
			}

			if(keyframeDifference)
			{
				if(SplineSolver != null)
					ScriptableObject.DestroyImmediate(SplineSolver);

				if(keyframes.Count == 2)
					SplineSolver = ScriptableObject.CreateInstance<LinearSplineSolver>();
				else if(keyframes.Count == 3)
					SplineSolver = ScriptableObject.CreateInstance<QuadraticSplineSolver>();
				else if(keyframes.Count == 4)
					SplineSolver = ScriptableObject.CreateInstance<CubicBezierSplineSolver>();
				else
					SplineSolver = ScriptableObject.CreateInstance<CatmullRomSplineSolver>();
			}

			SplineSolver.Nodes = keyframes;
			SplineSolver.Build();
		}

		private Vector3 GetPosition(float time)
		{
			return SplineSolver.GetPosition(time);
		}

		public Vector3 GetPositionOnPath(float time)
		{
			if(time < 0.0f || time > 1.0f)
			{
				if(IsClosed)
				{
					if(time < 0.0f)
						time += 1.0f;
					else
						time -= 1.0f;
				}
				else
					time = Mathf.Clamp01(time);
			}
			
			return SplineSolver.GetPositionOnPath(time);
		}

		public void Close()
		{
			if(IsClosed)
				throw new System.Exception("Closing a Spline that is already closed");
			
			IsClosed = true;
			SplineSolver.Close();
		}

		public void Reverse()
		{
			if(!IsReversed)
			{
				SplineSolver.Reverse();
				IsReversed = true;
			}
			else
			{
				SplineSolver.Reverse();
				IsReversed = false;
			}
		}

		public void OnDrawGizmos()
		{
			if(SplineSolver == null)
				return;
			
			SplineSolver.OnInternalDrawGizmos(SplineColor, DisplayResolution);
		}
	}
}