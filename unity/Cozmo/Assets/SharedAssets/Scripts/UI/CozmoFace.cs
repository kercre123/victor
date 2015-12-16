using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using CozmoAnimation;
using System.Collections.Generic;
using Newtonsoft.Json;

[RequireComponent(typeof(RawImage))]
public class CozmoFace : MonoBehaviour {

  private static readonly IDAS sDAS = DAS.GetInstance(typeof(CozmoFace));

  private static CozmoFace _Instance;

  private Vector2 _LastFaceCenter;
  private Vector2 _LastFaceScale = Vector2.one;
  private float _LastFaceAngle; 
  private readonly ProceduralEyeParameters _LastLeftEye = ProceduralEyeParameters.MakeDefaultLeftEye();
  private readonly ProceduralEyeParameters _LastRightEye = ProceduralEyeParameters.MakeDefaultRightEye();

  private readonly ProceduralEyeParameters _LeftEye = ProceduralEyeParameters.MakeDefaultLeftEye();
  private readonly ProceduralEyeParameters _RightEye = ProceduralEyeParameters.MakeDefaultRightEye();

  public ProceduralEyeParameters LeftEye { get { return _LeftEye; } }
  public ProceduralEyeParameters RightEye { get { return _RightEye; } }
  public Vector2 FaceCenter { get; set; }
  public Vector2 FaceScale { get; set; }
  public float FaceAngle { get; set; }

  public RawImage Image;

  private float _BlinkTimer;

  private List<KeyFrame> _CurrentSequence;
  private long _CurrentSequenceStartTime;
  private int _CurrentSequenceIndex;
  private System.Action _CurrentSequenceCompleteCallback;
  private bool _CurrentSequenceRelative;

  private static readonly Dictionary<string, List<KeyFrame>> _Animations = new Dictionary<string, List<KeyFrame>>();

  private static List<KeyFrame> LoadAnimation(string name) {
    
    string fileName = "";
#if UNITY_EDITOR
    fileName = Application.dataPath + string.Format("/../../../lib/anki/products-cozmo-assets/animations/{0}.json", name);
#elif UNITY_IOS
    fileName = Application.dataPath + string.Format("/../cozmo_resources/assets/animations/{0}.json", name);
#endif
    try {
      var jsonText = System.IO.File.ReadAllText(fileName);

      var sequenceWrapper = Newtonsoft.Json.JsonConvert.DeserializeObject<Dictionary<string, List<KeyFrame>>>(
        jsonText,
        new JsonSerializerSettings() {
          TypeNameHandling = TypeNameHandling.None,
          Converters = new List<JsonConverter> {
            new UtcDateTimeConverter(),
            new Newtonsoft.Json.Converters.StringEnumConverter()
          }
        });

      var sequence = sequenceWrapper[name];

      // drop everything thats not face
      sequence.RemoveAll(x => x.Name != KeyFrameName.ProceduralFaceKeyFrame);
      sequence.Sort((x, y) => {
        return x.triggerTime_ms.CompareTo(y.triggerTime_ms);
      });
        
      return sequence;
    }
    catch(System.Exception ex) {
      sDAS.Error("Failed to load Animation " + name);
      sDAS.Error(ex);
      return null;
    }
  }

  public static void DisplayProceduralFace(float faceAngle, Vector2 faceCenter, Vector2 faceScale, float[] leftEyeParams, float[] rightEyeParams) {
    if (_Instance != null) {
      _Instance.FaceAngle = faceAngle;
      _Instance.FaceCenter = faceCenter;
      _Instance.FaceScale = faceScale;
      leftEyeParams.CopyTo(_Instance.LeftEye.Array, 0);
      rightEyeParams.CopyTo(_Instance.RightEye.Array, 0);

      _Instance._LastFaceAngle = faceAngle;
      _Instance._LastFaceCenter = faceCenter;
      _Instance._LastFaceScale = faceScale;
      leftEyeParams.CopyTo(_Instance._LastLeftEye.Array, 0);
      rightEyeParams.CopyTo(_Instance._LastRightEye.Array, 0);
    }
  }

  public static void PlayAnimation(string name, bool relative = false, System.Action callback = null) {
    List<KeyFrame> anim;
    if (!_Animations.TryGetValue(name, out anim)) {
      anim = LoadAnimation(name);
      _Animations[name] = anim;
    }
    if (_Instance != null) {
      _Instance.PlayAnimation(anim, relative, callback);
    }
  }


  private void Awake() {
    _Instance = this;
    FaceScale = Vector2.one;
    // clone our face material to avoid making changes to the original
    Image.material = new Material(Image.material) { hideFlags = HideFlags.HideAndDontSave };
  }

  private void Update() {
    UpdateBlink();
    UpdateSequence();
    UpdateMaterial();
  }

  private void PlayAnimation(List<KeyFrame> sequence, bool relative = false, System.Action callback = null) {

    _CurrentSequence = sequence;
    _CurrentSequenceRelative = relative;
    _CurrentSequenceIndex = 0;
    _CurrentSequenceStartTime = System.DateTime.UtcNow.ToUtcMs();
    _CurrentSequenceCompleteCallback = callback;
  }

  private void UpdateBlink() {
    if(_CurrentSequence == null) {
      _BlinkTimer -= Time.deltaTime;
      if (_BlinkTimer < 0) {
        PlayAnimation("procedural_blink", true);
        _BlinkTimer = UnityEngine.Random.Range(3f, 4f);
      }
    }
  }

  private void UpdateSequence() {
    if (_CurrentSequence == null) {

      // lerp back to our displayed position after finishing the animation
      if (_CurrentSequenceRelative) {
        float lerpAmount = 15f * Time.deltaTime;

        // lerp to default position
        ArrayLerp(_LeftEye.Array, _LastLeftEye.Array, lerpAmount, _LeftEye.Array);
        ArrayLerp(_RightEye.Array, _LastRightEye.Array, lerpAmount, _RightEye.Array);
        FaceScale = Vector2.Lerp(FaceScale, _LastFaceScale, lerpAmount);
        FaceCenter = Vector2.Lerp(FaceCenter, _LastFaceCenter, lerpAmount);
        FaceAngle = Mathf.Lerp(FaceAngle, _LastFaceAngle, lerpAmount);
      }
      return;
    }

    var time = System.DateTime.UtcNow.ToUtcMs() - _CurrentSequenceStartTime;

    bool done = (_CurrentSequenceIndex == _CurrentSequence.Count);
  
    if(!done) {
      
      var frame = _CurrentSequence[_CurrentSequenceIndex];
      var nextFrame = _CurrentSequenceIndex < _CurrentSequence.Count - 1 ? _CurrentSequence[_CurrentSequenceIndex + 1] : null;
      // if there is a next frame, check if we have reached its trigger time and move to it
      if (nextFrame != null && time > nextFrame.triggerTime_ms) {
        frame = nextFrame;
        _CurrentSequenceIndex++;
        nextFrame = _CurrentSequenceIndex < _CurrentSequence.Count - 1 ? _CurrentSequence[_CurrentSequenceIndex + 1] : null;
      }

      // if the time is in between two frames, lerp between them
      if (time > frame.durationTime_ms + frame.triggerTime_ms) {
        // if there is no next frame and we have exceeded the duration of the last key frame, we are done. Set the
        // last frame as the current face first though.
        if (nextFrame == null) {
          _CurrentSequenceIndex++;
          nextFrame = frame;
        }

        float denominator = nextFrame.triggerTime_ms - frame.triggerTime_ms - frame.durationTime_ms;
        float t = denominator == 0 ? 0 : (time - frame.triggerTime_ms - frame.durationTime_ms) / denominator;

        FaceCenter = Vector2.Lerp(new Vector2(frame.faceCenterX, frame.faceCenterY), 
          new Vector2(nextFrame.faceCenterX, nextFrame.faceCenterY), 
          t);
        FaceScale = Vector2.Lerp(new Vector2(frame.faceScaleX, frame.faceScaleY), 
          new Vector2(nextFrame.faceScaleX, nextFrame.faceScaleY), 
          t);
        FaceAngle = Mathf.Lerp(frame.faceAngle, nextFrame.faceAngle, t);

        ArrayLerp(frame.leftEye, nextFrame.leftEye, t, _LeftEye.Array);
        ArrayLerp(frame.rightEye, nextFrame.rightEye, t, _RightEye.Array);
      
      }
      // otherwise we are on a keyframe with duration, hold it.
      else if (time > frame.triggerTime_ms) {
        FaceCenter = new Vector2(frame.faceCenterX, frame.faceCenterY); 
        FaceScale = new Vector2(frame.faceScaleX, frame.faceScaleY);
        FaceAngle = frame.faceAngle;
        frame.leftEye.CopyTo(_LeftEye.Array, 0);
        frame.rightEye.CopyTo(_RightEye.Array, 0);
      }

      if (_CurrentSequenceRelative) {
        _LeftEye.EyeCenter += _LastLeftEye.EyeCenter;
        _LeftEye.EyeScale = new Vector2(_LeftEye.EyeScale.x * _LastLeftEye.EyeScale.x, _LeftEye.EyeScale.y * _LastLeftEye.EyeScale.y);

        _LeftEye.LowerLidAngle = _LastLeftEye.LowerLidAngle;
        _LeftEye.LowerLidBend = _LastLeftEye.LowerLidBend;
        _LeftEye.LowerLidY = _LastLeftEye.LowerLidY;
        _LeftEye.UpperLidAngle = _LastLeftEye.UpperLidAngle;
        _LeftEye.UpperLidBend = _LastLeftEye.UpperLidBend;
        _LeftEye.UpperLidY = _LastLeftEye.UpperLidY;

        _RightEye.EyeCenter += _LastRightEye.EyeCenter;
        _RightEye.EyeScale = new Vector2(_RightEye.EyeScale.x * _LastRightEye.EyeScale.x, _RightEye.EyeScale.y * _LastRightEye.EyeScale.y);

        _RightEye.LowerLidAngle = _LastRightEye.LowerLidAngle;
        _RightEye.LowerLidBend = _LastRightEye.LowerLidBend;
        _RightEye.LowerLidY = _LastRightEye.LowerLidY;
        _RightEye.UpperLidAngle = _LastRightEye.UpperLidAngle;
        _RightEye.UpperLidBend = _LastRightEye.UpperLidBend;
        _RightEye.UpperLidY = _LastRightEye.UpperLidY;

        FaceAngle += _LastFaceAngle;
        FaceCenter += _LastFaceCenter;
        FaceScale = new Vector2(FaceScale.x * _LastFaceScale.x, FaceScale.y * _LastFaceScale.y);
      }
    }
    else {
      // Reached End of animation
      _CurrentSequence = null;
      var callback = _CurrentSequenceCompleteCallback;
      _CurrentSequenceCompleteCallback = null;

      // if this wasn't a relative animation, update our last face to the current face
      if (!_CurrentSequenceRelative) {
        _LastFaceCenter = FaceCenter;
        _LastFaceScale = FaceScale;
        _LastFaceAngle = FaceAngle;
        _LeftEye.Array.CopyTo(_LastLeftEye.Array, 0);
        _RightEye.Array.CopyTo(_LastRightEye.Array, 0);
      }

      if (callback != null) {
        callback();
      }
    }
  }

  private void ArrayLerp(float[] start, float[] end, float t, float[] result) {
    for (int i = 0; i < start.Length; i++) {
      result[i] = Mathf.Lerp(start[i], end[i], t);
    }
  }

  private void UpdateMaterial() {
    var material = Image.material;

    material.SetVector("_FaceCenterScale", 
      new Vector4(FaceCenter.x, FaceCenter.y, FaceScale.x, FaceScale.y));
    material.SetFloat("_FaceAngle", FaceAngle);

    LeftEye.SetMaterialValues(material, left:true);
    RightEye.SetMaterialValues(material, left:false);
  }
}
