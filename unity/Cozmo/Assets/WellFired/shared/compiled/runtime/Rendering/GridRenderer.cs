using UnityEngine;
using System.Collections;

namespace WellFired.Shared
{
	public static class GridRenderer
	{
		private static string defaultShader = "Shader \"Lines/Colored Blended\" {" +
			"SubShader { Pass { " +
				"	Blend SrcAlpha OneMinusSrcAlpha " +
				"	ZWrite Off Cull Off Fog { Mode Off } " +
				"	BindChannels {" +
				"	Bind \"vertex\", vertex Bind \"color\", color }" +
				"} } }";
	
		private static Material cachedDefaultRenderMaterial;
		private static Material defaultRenderMaterial 
		{
			get 
			{
				if(!cachedDefaultRenderMaterial)
					cachedDefaultRenderMaterial = new Material(defaultShader);
	
				return cachedDefaultRenderMaterial;
			}
		}
	
		/// <summary>
		/// Renders a grid.
		/// You should call this from inside an OnPostRender Method
		/// <c><a href="http://docs.unity3d.com/Documentation/ScriptReference/MonoBehaviour.OnPostRender.html">OnPostRender</a></c> method.
		/// </summary>
		public static void RenderGrid(bool[,] grid, Color color, Vector2 origin, float gridSpacing, float lineWidth, Camera camera)
		{
			RenderGridLines(grid, color, origin, gridSpacing, lineWidth, camera, defaultRenderMaterial);
		}
		
		private static void RenderGridLines(bool[,] grid, Color color, Vector2 origin, float gridSpacing, float lineWidth, Camera camera, Material renderMaterial)
		{
			var cameraTransform = camera.transform;

			var width = (int)grid.GetUpperBound(0) + 1;
			var height = (int)grid.GetUpperBound(0) + 1;
			
			var drawPoints = new Vector3[width, height];

			for(var x = 0; x < width; x++)
			{
				var xPos = origin.x + (x * gridSpacing);
				for(var y = 0; y < height; y++)
				{
					var yPos = origin.y + (y * gridSpacing);

					var sourceX = xPos;
					var sourceY = yPos;

					drawPoints[x, y] = new Vector3(sourceX, sourceY);
				}
			}

			renderMaterial.SetPass(0);

			GL.Begin(GL.QUADS);
			GL.Color(color);

			float mult = Mathf.Max(0, 0.5f * lineWidth);

			for(int axis = 0; axis < 2; axis++)
			{
				for(var x = 0; x < width - 1; x++)
				{
					for(var y = 0; y < height - 1; y++)
					{
						var shouldRender = grid[x, y];
					
						if(!shouldRender)
							continue;
					
						var point1 = drawPoints[x, y];
						var point2 = axis == 0 ? drawPoints[x + 1, y] : drawPoints[x, y + 1];
						var point3 = axis == 0 ? drawPoints[x, y + 1] : drawPoints[x + 1, y];
						var point4 = drawPoints[x + 1, y + 1];
				
						var dir = Vector3.Cross(point1 - point2, cameraTransform.forward).normalized;
					
						if(camera.orthographic)
						{
							dir *= (camera.orthographicSize * 2) / camera.pixelHeight;
						} 
						else
						{
							dir *= (camera.ScreenToWorldPoint(new Vector3(0, 0, 50)) - camera.ScreenToWorldPoint(new Vector3(20, 0, 50))).magnitude/20;
						}
					
						GL.Vertex(point1 - mult * dir);
						GL.Vertex(point1 + mult * dir);
						GL.Vertex(point2 + mult * dir);
						GL.Vertex(point2 - mult * dir);
						
						GL.Vertex(point3 - mult * dir);
						GL.Vertex(point3 + mult * dir);
						GL.Vertex(point4 + mult * dir);
						GL.Vertex(point4 - mult * dir);
					}
				}
			}

			GL.End();
		}
	}
}