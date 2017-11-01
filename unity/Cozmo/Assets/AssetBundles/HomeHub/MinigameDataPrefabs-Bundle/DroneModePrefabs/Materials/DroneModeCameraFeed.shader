// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "UI/Cozmo/DroneModeCameraFeed"
{
	Properties {
		// Grayscale camera feed texture from Cozmo
		_MainTex ("Base (RGB)", 2D) = "white" {}

		// Constant; B&W Texture to use for scanline effect; multiplied with the feed texture 
		_ScanTex ("Scanline texture", 2D) = "white" {}

		// Constant 0-1; Value denoting "strength" or "opacity" of the scan lines
		_ScanBlend ("Scanline blending", Float) = 0.5

		// Constant; Number of times to repeat the texture in the feed; larger = finer lines
		_ScanSize ("Scanline Size", Float) = 1

		// Constant; Color of the line effect lerps from top to bottom
		_TopColor ("Gradient Top Color", Color) = (0.5,0.5,1,1)
		_BottomColor ("Gradient Bottom Color", Color) = (0.25,0.25,0.5,1)

		// Constant 0-1; Strength, in uv units, to apply a the up-down jittering animation
		_UVOffsetOverTime ("UV Offset", Float) = 0.25
	} 
	SubShader {
        Tags
        { 
            "Queue"="Transparent" 
            "IgnoreProjector"="True" 
            "RenderType"="Transparent" 
            "CanUseSpriteAtlas"="True"
        }

        Blend SrcAlpha OneMinusSrcAlpha
        Cull Off

		Pass {
			CGPROGRAM
	        #pragma vertex vert
	        #pragma fragment frag
			#include "UnityCG.cginc"
			
			// Unity requires that the properties above are re-defined in the cg block with the same name
			uniform sampler2D _MainTex;
			uniform sampler2D _ScanTex;
			
			fixed _ScanBlend;
			fixed _ScanSize;

			fixed4 _TopColor;
			fixed4 _BottomColor;

			fixed _UVOffsetOverTime;

	        struct appdata {
	            fixed4 vertex : POSITION;
	            fixed4 texcoord : TEXCOORD0;
	        };

	        struct v2f {
	            fixed4 pos : SV_POSITION;
	            fixed4 uv : TEXCOORD0;
	            fixed4 uv2 : TEXCOORD1;
	            fixed4 color : COLOR;
	        };
	        

	        // Vertex shader

	        v2f vert (appdata v) {
	        	// Basic vertex position and uv coordinate setting
	            v2f o;
	            o.pos = UnityObjectToClipPos( v.vertex );
	            o.uv = fixed4( v.texcoord.xy, 0, 0 );

	            // Create a gradient effect from top to bottom
	            o.color = lerp(_BottomColor, _TopColor, o.uv.y);

	            // Apply a uv offset based on animation curve and strength constant
	            // http://www.iquilezles.org/apps/graphtoy/
	            fixed graphedTime = (0.5*(sin(10*4*_Time.x)-(sin(14*_Time.x))));
	            o.uv2 = o.uv + graphedTime * _UVOffsetOverTime;
	            return o;
	        }
	        
	        
	        // Fragment shader

	        fixed4 frag( v2f i ) : SV_Target {
	        	// Sample textures for values based on uvs
				fixed4 base = tex2D(_MainTex, i.uv);
				fixed4 scan = tex2D(_ScanTex, i.uv2 * _ScanSize);

				// Multiply by scan line and color for line effect
				base = base * (scan * _ScanBlend) * i.color;

				return base;
	        }
	        ENDCG
		}
	}
}
