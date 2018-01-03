// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

// Meant for use with an empty Image (CozmoImage, Image, RawImage). Lerps pixel color linearly 
// between _LeftColor and _RightColor from - you guessed it - left to right.
Shader "UI/Cozmo/HorizontalGradientUIShader"
{
  Properties 
  {
    _LeftColor ("Left Color", Color) = (1,1,1,1)
    _RightColor ("Right Color", Color) = (0,0,0,1)
  }
  SubShader 
  {
    Tags {"Queue"="Transparent" "IgnoreProjector"="True" "RenderType"="Transparent"}
    Cull Off
    ZWrite Off
	  Blend SrcAlpha OneMinusSrcAlpha 

    Pass 
    {
      CGPROGRAM
      #pragma vertex vert  
      #pragma fragment frag
      #include "UnityCG.cginc"
      
      fixed4 _LeftColor;
      fixed4 _RightColor;

      struct v2f 
      {
        float4 pos : SV_POSITION;
        fixed4 col : COLOR;
      };

      v2f vert (appdata_full v)
      {
        v2f o;
        o.pos = UnityObjectToClipPos (v.vertex);

        // Map vertex color based on x value
        o.col = lerp(_LeftColor,_RightColor, v.texcoord.x );

        return o;
      }

      float4 frag (v2f i) : COLOR 
      {
        // Color automatically lerps from vert to vert; no need to do anything
        return i.col;
      }
      ENDCG
    }
  }
}
