Shader "UI/Cozmo/DroneModeCameraFeed"
{
	Properties {
		_MainTex ("Base (RGB)", 2D) = "white" {}
		_MaskAlphaTex ("Mask texture", 2D) = "white" {}
		_ScanTex ("Scanline texture", 2D) = "white" {}
		_ScanBlend ("Scanline blending", Float) = 0.5
		_ScanSize ("Scanline Size", Float) = 1
		_TopColor ("Gradient Top Color", Color) = (0.5,0.5,1,1)
		_BottomColor ("Gradient Bottom Color", Color) = (0.25,0.25,0.5,1)
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

		Pass {
			CGPROGRAM
	        #pragma vertex vert
	        #pragma fragment frag
			#include "UnityCG.cginc"
		
			uniform sampler2D _MainTex;
			uniform sampler2D _MaskAlphaTex;
			uniform sampler2D _ScanTex;
			
			fixed _ScanBlend;
			fixed _ScanSize;
			fixed _UVOffsetOverTime;

			fixed4 _TopColor;
			fixed4 _BottomColor;

	        // vertex input: position, UV
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
	        
	        v2f vert (appdata v) {
	            v2f o;
	            o.pos = mul( UNITY_MATRIX_MVP, v.vertex );
	            o.uv = fixed4( v.texcoord.xy, 0, 0 );
	            o.color = lerp(_BottomColor, _TopColor, o.uv.y);

	            // http://www.iquilezles.org/apps/graphtoy/
	            fixed graphedTime = (0.5*(sin(10*4*_Time.x)-(sin(14*_Time.x))));
	            o.uv2 = o.uv + graphedTime * _UVOffsetOverTime;
	            return o;
	        }
	        
	        fixed4 frag( v2f i ) : SV_Target {
				fixed4 base = tex2D(_MainTex, i.uv);
				fixed4 scan = tex2D(_ScanTex, i.uv2 * _ScanSize);
				fixed4 mask = tex2D(_MaskAlphaTex, i.uv);

				// Manually grayscale
				fixed lum = base.r*.3 + base.g*.59 + base.b*.11;
				base = fixed4( lum, lum, lum, base.a ); 

				// Scan line effect 1 - blend
				// base = lerp(base, scan, _ScanBlend) * i.color;

				// Multiply by scan line and color
				base = base * (scan * _ScanBlend) * i.color;
				base.a = mask.a;

				return base;
	        }
	        ENDCG
		}
	}
}
