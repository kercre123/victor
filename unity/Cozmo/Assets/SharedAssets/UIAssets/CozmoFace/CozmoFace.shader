Shader "UI/CozmoFace"
{
	Properties
	{
    _FaceAngle ("Face Angle", Float) = 0
    _FaceCenterScale ("Face Center Scale", Vector) = (0, 0, 1, 1)

    _LeftEyeCenterScale ("Left Eye Center Scale", Vector) = (0,0,1,1)
    _LeftEyeAngle ("Left Eye Angle", Float) = 0
    _LeftInnerRadius ("Left Inner Radius", Vector) = (0,0,0,0)
    _LeftOuterRadius ("Left Outer Radius", Vector) = (0,0,0,0)

    _LeftUpperLidY ("Left Upper Lid Y", Float) = 0
    _LeftUpperLidAngle ("Left Upper Lid Angle", Float) = 0
    _LeftUpperLidBend ("Left Upper Lid Bend", Float) = 0

    _LeftLowerLidY ("Left Lower Lid Y", Float) = 0
    _LeftLowerLidAngle ("Left Lower Lid Angle", Float) = 0
    _LeftLowerLidBend ("Left Lower Lid Bend", Float) = 0

    _RightEyeCenterScale ("Right Eye Center Scale", Vector) = (0,0,1,1)
    _RightEyeAngle ("Right Eye Angle", Float) = 0
    _RightInnerRadius ("Right Inner Radius", Vector) = (0,0,0,0)
    _RightOuterRadius ("Right Outer Radius", Vector) = (0,0,0,0)

    _RightUpperLidY ("Right Upper Lid Y", Float) = 0
    _RightUpperLidAngle ("Right Upper Lid Angle", Float) = 0
    _RightUpperLidBend ("Right Upper Lid Bend", Float) = 0

    _RightLowerLidY ("Right Lower Lid Y", Float) = 0
    _RightLowerLidAngle ("Right Lower Lid Angle", Float) = 0
    _RightLowerLidBend ("Right Lower Lid Bend", Float) = 0

	}
	SubShader
	{
		// No culling or depth
		Cull Off ZWrite Off ZTest Always

		Pass
		{
			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			
			#include "UnityCG.cginc"

			struct appdata
			{
				float4 vertex : POSITION;
				float2 uv : TEXCOORD0;
			};

			struct v2f
			{
				float2 uv : TEXCOORD0;
				float4 vertex : SV_POSITION;
        float4 tangent : TANGENT;
        float4 sine : TEXCOORD1;
        float4 cosine : TEXCOORD2;
        float3 face : NORMAL;
        float4 eyes : TEXCOORD3;
			};

			sampler2D _MainTex;
      uniform fixed4 _Color;

      float _FaceAngle;
      float4 _FaceCenterScale;

      float4 _LeftEyeCenterScale;
      float _LeftEyeAngle;
      float4 _LeftInnerRadius;
      float4 _LeftOuterRadius;

      float _LeftUpperLidY;
      float _LeftUpperLidAngle;
      float _LeftUpperLidBend;

      float _LeftLowerLidY;
      float _LeftLowerLidAngle;
      float _LeftLowerLidBend;

      float4 _RightEyeCenterScale;
      float _RightEyeAngle;
      float4 _RightInnerRadius;
      float4 _RightOuterRadius;

      float _RightUpperLidY;
      float _RightUpperLidAngle;
      float _RightUpperLidBend;

      float _RightLowerLidY;
      float _RightLowerLidAngle;
      float _RightLowerLidBend;

      v2f vert (appdata v)
      {
        v2f o;
        o.vertex = mul(UNITY_MATRIX_MVP, v.vertex);
        o.uv = v.uv;

        o.sine = float4(sin(radians(_LeftUpperLidAngle)), sin(radians(_LeftLowerLidAngle)),
                        sin(-radians(_RightUpperLidAngle)), sin(-radians(_RightLowerLidAngle)));

        o.cosine = float4(cos(radians(_LeftUpperLidAngle)), cos(radians(_LeftLowerLidAngle)),
                          cos(-radians(_RightUpperLidAngle)), cos(-radians(_RightLowerLidAngle)));

        o.tangent = float4(tan(radians(_LeftUpperLidAngle)), tan(radians(_LeftLowerLidAngle)),
                           tan(-radians(_RightUpperLidAngle)), tan(-radians(_RightLowerLidAngle)));

        o.face = float3(sin(-radians(_FaceAngle)), cos(-radians(_FaceAngle)), 0);

        o.eyes = float4(sin(-radians(_LeftEyeAngle)), cos(-radians(_LeftEyeAngle)), 
                        sin(-radians(_RightEyeAngle)), cos(-radians(_RightEyeAngle)));

        return o;
      }
      

      const float IMAGE_WIDTH = 128;
      const float IMAGE_HEIGHT = 64;

      const float2 IMAGE_SIZE = float2(IMAGE_WIDTH, IMAGE_HEIGHT);

      const float NominalEyeHeight       = 40;
      const float NominalEyeWidth        = 30;

      const float2 NominalEyeSize = float2(NominalEyeWidth, NominalEyeHeight);

      const float2 HalfNominalEyeSize = float2(NominalEyeWidth * 0.5, NominalEyeHeight * 0.5);

      const float NominalLeftEyeX        = 32;
      const float NominalRightEyeX       = 96;
      const float NominalEyeY            = 32;

      const float2 NominalLeftEyePosition = float2(NominalLeftEyeX, NominalEyeY);
      const float2 NominalRightEyePosition = float2(NominalRightEyeX, NominalEyeY);

      float mod(float x, float d)
      {
        return x - floor(x / d) * d;
      }

      // pos is relative to eye position
      float eyeContains(float2 pos, float2 tl, float2 tr, float2 bl, float2 br, float2 scale) {

        float2 halfS = scale * HalfNominalEyeSize;

        tl = tl * halfS;
        tr = tr * halfS;
        bl = bl * halfS;
        br = br * halfS;

        float2 bottomRight = halfS;
        float2 topLeft = -halfS;

        float2 bottomLeft = float2(topLeft.x, bottomRight.y);
        float2 topRight = float2(bottomRight.x, topLeft.y);

        float2 innerTopLeft = topLeft + tl;
        float2 innerTopRight = topRight + float2(-tr.x, tr.y);
        float2 innerBottomLeft = bottomLeft + float2(bl.x, -bl.y);
        float2 innerBottomRight = bottomRight - br;

        if(pos.x > topRight.x || pos.x < bottomLeft.x)
          return 0;

        if(pos.y < topRight.y || pos.y > bottomLeft.y)
          return 0;

        //top left
        float2 r = pos - innerTopLeft;
        float2 d2 = (r * r) / (tl * tl);
        if(d2.x + d2.y > 1 && r.x < 0 && r.y < 0)
          return 0;

        // top right
        r = pos - innerTopRight;
        d2 = (r * r) / (tr * tr);
        if(d2.x + d2.y > 1 && r.x > 0 && r.y < 0)
          return 0;

        // bottom left
        r = pos - innerBottomLeft;
        d2 = (r * r) / (bl * bl);
        if(d2.x + d2.y > 1 && r.x < 0 && r.y > 0)
          return 0;

        // bottom right
        r = pos - innerBottomRight;
        d2 = (r * r) / (br * br);
        if(d2.x + d2.y > 1 && r.x > 0 && r.y > 0)
          return 0;

        return 1;
      }

      float bottomEyelidContains(float2 pos, float lidY, float sina, float cosa, float tana, float bend, float2 scale) {

        float eyeHeight = scale.y * NominalEyeHeight;
        float offset = eyeHeight * (0.5 - lidY);

        float2 point = float2(0, offset);

        float slope = tana;

        float2 delta = pos - point;

        if(delta.y > slope * delta.x)
          return 1;

        bend = bend * eyeHeight;
        if(bend == 0)
          return 0;

        float bendWidth = 0.5 * scale.x * NominalEyeWidth / cosa;

        float leftBranch = delta.x * cosa + delta.y * sina;
        float rightBranch = delta.x * sina - delta.y * cosa;

        if((leftBranch * leftBranch) / (bendWidth * bendWidth) + 
           (rightBranch * rightBranch) / (bend * bend) < 1)
           return 1;

         return 0;
      }

      float topEyelidContains(float2 pos, float lidY, float sina, float cosa, float tana, float bend, float2 scale) {

        float eyeHeight = scale.y * NominalEyeHeight;
        float offset = eyeHeight * (lidY - 0.5);

        float2 point = float2(0, offset);

        float slope = tana;

        float2 delta = pos - point;

        if(delta.y < slope * delta.x)
          return 1;

        bend = bend * eyeHeight;
        if(bend == 0)
          return 0;

        float bendWidth = 0.5 * scale.x * NominalEyeWidth / cosa;

        float leftBranch = delta.x * cosa + delta.y * sina;
        float rightBranch = delta.x * sina - delta.y * cosa;

        if((leftBranch * leftBranch) / (bendWidth * bendWidth) + 
           (rightBranch * rightBranch) / (bend * bend) < 1)
           return 1;

         return 0;
      }

      fixed4 frag (v2f i) : SV_Target
      {
        float2 imagePos = IMAGE_SIZE * i.uv * float2(1, -1) + float2(0, IMAGE_HEIGHT);

        float4 baseColor = float4(1,1,1,1);

        float2 facePos = (imagePos - _FaceCenterScale.xy - IMAGE_SIZE * 0.5) / _FaceCenterScale.zw;

        // apply rotation

        facePos = float2(facePos.x * i.face.y + facePos.y * i.face.x, 
                         facePos.y * i.face.y - facePos.x * i.face.x) + IMAGE_SIZE * 0.5;

        float2 leftEyePos = _LeftEyeCenterScale.xy + NominalLeftEyePosition;

        float2 leftPos = facePos - leftEyePos;

        // apply rotation
        leftPos = float2(leftPos.x * i.eyes.y + leftPos.y * i.eyes.x, leftPos.y * i.eyes.y - leftPos.x * i.eyes.x);

        float left = eyeContains(leftPos, 
                                 _LeftOuterRadius.xy, 
                                 _LeftInnerRadius.zw, 
                                 _LeftOuterRadius.zw, 
                                 _LeftInnerRadius.xy, 
                                 _LeftEyeCenterScale.zw);

        float leftEyelidTop = topEyelidContains(leftPos, 
                                                _LeftUpperLidY, 
                                                i.sine.x, 
                                                i.cosine.x, 
                                                i.tangent.x, 
                                                _LeftUpperLidBend, 
                                                _LeftEyeCenterScale.zw);
        float leftEyelidBottom = bottomEyelidContains(leftPos, 
                                                      _LeftLowerLidY, 
                                                      i.sine.y, 
                                                      i.cosine.y, 
                                                      i.tangent.y, 
                                                      _LeftLowerLidBend, 
                                                      _LeftEyeCenterScale.zw);

        float2 rightEyePos = _RightEyeCenterScale.xy + NominalRightEyePosition;

        float2 rightPos = facePos - rightEyePos;

        // apply rotation
        rightPos = float2(rightPos.x * i.eyes.w + rightPos.y * i.eyes.z, 
                          rightPos.y * i.eyes.w - rightPos.x * i.eyes.z);

        float right = eyeContains(rightPos, 
                                  _RightInnerRadius.zw, 
                                  _RightOuterRadius.xy, 
                                  _RightInnerRadius.xy, 
                                  _RightOuterRadius.zw, 
                                  _RightEyeCenterScale.zw);

        float rightEyelidTop = topEyelidContains(rightPos, 
                                                 _RightUpperLidY, 
                                                 i.sine.z, 
                                                 i.cosine.z, 
                                                 i.tangent.z, 
                                                 _RightUpperLidBend, 
                                                 _RightEyeCenterScale.zw);
        float rightEyelidBottom = bottomEyelidContains(rightPos, 
                                                       _RightLowerLidY, 
                                                       i.sine.w, 
                                                       i.cosine.w, 
                                                       i.tangent.w, 
                                                       _RightLowerLidBend, 
                                                       _RightEyeCenterScale.zw);

        left = saturate(left - leftEyelidTop - leftEyelidBottom);
        right = saturate(right - rightEyelidTop - rightEyelidBottom);

        return saturate(left + right) * baseColor;
      }
			ENDCG
		}
	}
}
