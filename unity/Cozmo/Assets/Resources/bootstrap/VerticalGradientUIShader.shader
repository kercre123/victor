// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

// Meant for use with an empty Image (CozmoImage, Image, RawImage). Lerps pixel color linearly 
// between _Color and _Color2 from top to bottom.
Shader "UI/Cozmo/VerticalGradientUIShader"
{
  Properties 
  {
    _Color ("Top Color", Color) = (1,1,1,1)
    _Color2 ("Bottom Color", Color) = (0,0,0,1)
  }
  SubShader 
  {
    ZWrite On
	  Blend SrcAlpha OneMinusSrcAlpha 

    Pass 
    {
      CGPROGRAM
      #pragma vertex vert  
      #pragma fragment frag
      #include "UnityCG.cginc"
      
      fixed4 _Color;
      fixed4 _Color2;

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
        o.col = lerp(_Color,_Color2, 1.0 - v.texcoord.y );
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
