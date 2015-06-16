using UnityEngine;
using System.Collections;

namespace WellFired.Shared
{
	public class GridRenderingCamera : MonoBehaviour 
	{
		public bool[,] Grid
		{
			get;
			set;
		}
		
		public float gridWidth = 1;
		public float gridSpacing = 2;
		public Vector2 origin = new Vector2(-10.0f, -10.0f);

		private Camera gridCamera;

		private void Start()
		{
			gridCamera = GetComponent<Camera>();
		}
	
		private void OnPostRender()
		{
			if(Grid == null)
				return;
	
			WellFired.Shared.GridRenderer.RenderGrid(Grid, Color.black, origin, gridSpacing, gridWidth, gridCamera);
		}
	}
}