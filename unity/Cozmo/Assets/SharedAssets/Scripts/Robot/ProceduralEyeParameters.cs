using System;
using UnityEngine;
using Anki.Cozmo;

public class ProceduralEyeParameters
{
  // See enum ProceduralEyeParameter for mapping
  private readonly float[] _Arr = new float[(int)ProceduralEyeParameter.NumParameters];

  public Vector2 EyeCenter {
    get {
      return new Vector2(_Arr[(int)ProceduralEyeParameter.EyeCenterX], _Arr[(int)ProceduralEyeParameter.EyeCenterY]);
    }
    set {
      _Arr[(int)ProceduralEyeParameter.EyeCenterX] = value.x;
      _Arr[(int)ProceduralEyeParameter.EyeCenterY] = value.y;
    }
  }

  public Vector2 EyeScale {
    get {
      return new Vector2(_Arr[(int)ProceduralEyeParameter.EyeScaleX], _Arr[(int)ProceduralEyeParameter.EyeScaleY]);
    }
    set {
      _Arr[(int)ProceduralEyeParameter.EyeScaleX] = value.x;
      _Arr[(int)ProceduralEyeParameter.EyeScaleY] = value.y;
    }
  }

  public float EyeAngle { 
    get { return _Arr[(int)ProceduralEyeParameter.EyeAngle]; } 
    set { _Arr[(int)ProceduralEyeParameter.EyeAngle] = value; } 
  }

  public Vector2 LowerInnerRadius {
    get {
      return new Vector2(_Arr[(int)ProceduralEyeParameter.LowerInnerRadiusX], _Arr[(int)ProceduralEyeParameter.LowerInnerRadiusY]);
    }
    set {
      _Arr[(int)ProceduralEyeParameter.LowerInnerRadiusX] = value.x;
      _Arr[(int)ProceduralEyeParameter.LowerInnerRadiusY] = value.y;
    }
  }

  public Vector2 UpperInnerRadius {
    get {
      return new Vector2(_Arr[(int)ProceduralEyeParameter.UpperInnerRadiusX], _Arr[(int)ProceduralEyeParameter.UpperInnerRadiusY]);
    }
    set {
      _Arr[(int)ProceduralEyeParameter.UpperInnerRadiusX] = value.x;
      _Arr[(int)ProceduralEyeParameter.UpperInnerRadiusX] = value.y;
    }
  }

  public Vector2 UpperOuterRadius {
    get {
      return new Vector2(_Arr[(int)ProceduralEyeParameter.UpperOuterRadiusX], _Arr[(int)ProceduralEyeParameter.UpperOuterRadiusY]);
    }
    set {
      _Arr[(int)ProceduralEyeParameter.UpperOuterRadiusX] = value.x;
      _Arr[(int)ProceduralEyeParameter.UpperOuterRadiusX] = value.y;
    }
  }

  public Vector2 LowerOuterRadius {
    get {
      return new Vector2(_Arr[(int)ProceduralEyeParameter.LowerOuterRadiusX], _Arr[(int)ProceduralEyeParameter.LowerOuterRadiusY]);
    }
    set {
      _Arr[(int)ProceduralEyeParameter.LowerOuterRadiusX] = value.x;
      _Arr[(int)ProceduralEyeParameter.LowerOuterRadiusY] = value.y;
    }
  }


  public float UpperLidY { 
    get { return _Arr[(int)ProceduralEyeParameter.UpperLidY]; } 
    set { _Arr[(int)ProceduralEyeParameter.UpperLidY] = value; } 
  }

  public float UpperLidAngle { 
    get { return _Arr[(int)ProceduralEyeParameter.UpperLidAngle]; } 
    set { _Arr[(int)ProceduralEyeParameter.UpperLidAngle] = value; } 
  }

  public float UpperLidBend { 
    get { return _Arr[(int)ProceduralEyeParameter.UpperLidBend]; } 
    set { _Arr[(int)ProceduralEyeParameter.UpperLidBend] = value; } 
  }

  public float LowerLidY { 
    get { return _Arr[(int)ProceduralEyeParameter.LowerLidY]; } 
    set { _Arr[(int)ProceduralEyeParameter.LowerLidY] = value; } 
  }

  public float LowerLidAngle { 
    get { return _Arr[(int)ProceduralEyeParameter.LowerLidAngle]; } 
    set { _Arr[(int)ProceduralEyeParameter.LowerLidAngle] = value; } 
  }

  public float LowerLidBend { 
    get { return _Arr[(int)ProceduralEyeParameter.LowerLidBend]; } 
    set { _Arr[(int)ProceduralEyeParameter.LowerLidBend] = value; } 
  }

  public ProceduralEyeParameters() { }

  public ProceduralEyeParameters(
    Vector2 eyeCenter, 
    Vector2 eyeScale, 
    float eyeAngle, 
    Vector2 lowerInnerRadius, 
    Vector2 upperInnerRadius, 
    Vector2 upperOuterRadius, 
    Vector2 lowerOuterRadius, 
    float upperLidY,
    float upperLidAngle,
    float upperLidBend,
    float lowerLidY,
    float lowerLidAngle,
    float lowerLidBend) {
    EyeCenter = eyeCenter;
    EyeScale = eyeScale;
    EyeAngle = eyeAngle;
    LowerInnerRadius = lowerInnerRadius;
    UpperInnerRadius = upperInnerRadius;
    UpperOuterRadius = upperOuterRadius;
    LowerOuterRadius = lowerInnerRadius;
    UpperLidY = upperLidY;
    UpperLidAngle = upperLidAngle;
    UpperLidBend = upperLidBend;
    LowerLidY = lowerLidY;
    LowerLidAngle = lowerLidAngle;
    LowerLidBend = lowerLidBend;
  }

  public float[] Array { 
    get { 
      return _Arr; 
    } 
  }

  public ProceduralEyeParameters(float[] arr) {
    _Arr = arr;
  }

  public void Update(float[] arr) {
    arr.CopyTo(_Arr, 0);
  }

  public static implicit operator float[](ProceduralEyeParameters pep) {
    return pep._Arr;
  }

  public static implicit operator ProceduralEyeParameters(float[] arr) {
    return new ProceduralEyeParameters(arr);
  }
    
  public static ProceduralEyeParameters MakeDefaultLeftEye() {
    return new ProceduralEyeParameters() {
      EyeCenter = new Vector2(10, 4),
      EyeScale = new Vector2(1.01f, 0.87f),

      LowerInnerRadius = new Vector2(0.68f, 0.61f),
      LowerOuterRadius = new Vector2(0.68f, 0.61f),
      UpperOuterRadius = new Vector2(0.61f, 0.61f),
      UpperInnerRadius = new Vector2(0.61f, 0.61f)
    };
  }
  public static ProceduralEyeParameters MakeDefaultRightEye() {
    return new ProceduralEyeParameters() {
      EyeCenter = new Vector2(-13, 3),
      EyeScale = new Vector2(1.19f, 1f),

      LowerInnerRadius = new Vector2(0.62f, 0.61f),
      LowerOuterRadius = new Vector2(0.61f, 0.61f),
      UpperOuterRadius = new Vector2(0.61f, 0.61f),
      UpperInnerRadius = new Vector2(0.61f, 0.61f)
    };
  }

}

