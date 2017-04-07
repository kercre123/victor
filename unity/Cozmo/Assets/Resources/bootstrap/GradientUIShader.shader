// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "UI/Cozmo/GradientUIShader"
{
  Properties 
  {
    _Color ("Top Color", Color) = (1,1,1,1)
    _Color2 ("Bottom Color", Color) = (0,0,0,1)
  }
  SubShader 
  {
    ZWrite On
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
        o.col = lerp(_Color,_Color2, 1.0 - v.texcoord.y );
        return o;
      }
      float4 frag (v2f i) : COLOR 
      {
        return i.col;
      }
      ENDCG
    }
  }
}
