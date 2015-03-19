Shader "Mobile/Legacy/Lightmap/Lightmap AlphaTest"
{
	Properties
	{
		_Color ("Main Color", Color) = (1,1,1,1)
		_MainTex ("Base (RGB)", 2D) = "white" {}
		_LightMap ("Lightmap (RGB)", 2D) = "white" { LightmapMode }
		_Cutoff ("Alpha cutoff", Range(0,1)) = 0.5	
	}

	SubShader
	{
		Pass
		{
			Name "BASE"				
			Alphatest Greater [_Cutoff]
			
			BindChannels {
				Bind "Vertex", vertex
				Bind "texcoord1", texcoord0 // lightmap uses 2nd uv
				Bind "texcoord", texcoord1 // main uses 1st uv
			}
			SetTexture [_LightMap] {
				combine texture
			}
			SetTexture [_MainTex] {
				combine texture * previous, texture
			}
		}
	}
}