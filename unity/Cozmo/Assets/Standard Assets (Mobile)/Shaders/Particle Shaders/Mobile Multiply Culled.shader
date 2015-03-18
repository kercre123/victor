Shader "Mobile/Particles/Multiply Culled" {
Properties {
	_MainTex ("Main Texture", 2D) = "white" {}
}

Category {
	Tags { "Queue" = "Transparent" }
	Blend Zero SrcColor
	Lighting Off ZWrite Off Fog { Mode Off }
	BindChannels {
		Bind "Color", color
		Bind "Vertex", vertex
		Bind "TexCoord", texcoord
	}
	
	// ---- Dual texture cards
	SubShader {
		Pass {
			SetTexture [_MainTex] {
				combine texture * primary
			}
			SetTexture [_MainTex] {
				constantColor (1,1,1,1)
				combine previous lerp (previous) constant
			}
		}
	}
	
	// ---- Single texture cards (does not do particle colors)
	SubShader {
		Pass {
			SetTexture [_MainTex] {
				constantColor (1,1,1,1)
				combine texture lerp(texture) constant
			}
		}
	}
}
}
