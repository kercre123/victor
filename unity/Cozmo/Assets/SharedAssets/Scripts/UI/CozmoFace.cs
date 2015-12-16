using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using CozmoAnimation;
using System.Collections.Generic;
using Newtonsoft.Json;

[RequireComponent(typeof(RawImage))]
public class CozmoFace : MonoBehaviour {

  private static CozmoFace _Instance;
  public static CozmoFace Instance {
    get {
      return _Instance;
    }
  }

  private readonly ProceduralEyeParameters _LeftEye = ProceduralEyeParameters.MakeDefaultLeftEye();
  private readonly ProceduralEyeParameters _RightEye = ProceduralEyeParameters.MakeDefaultRightEye();

  public ProceduralEyeParameters LeftEye { get { return _LeftEye; } }
  public ProceduralEyeParameters RightEye { get { return _RightEye; } }
  public Vector2 FaceCenter { get; set; }
  public Vector2 FaceScale { get; set; }
  public float FaceAngle { get; set; }

  public RawImage Image;

  private List<KeyFrame> _CurrentSequence;
  private long _CurrentSequenceStartTime;
  private int _CurrentSequenceIndex;
  private System.Action _CurrentSequenceCompleteCallback;

  private void Awake() {
    _Instance = this;
    FaceScale = Vector2.one;
    // clone our face material to avoid making changes to the original
    Image.material = new Material(Image.material) { hideFlags = HideFlags.HideAndDontSave };


    // TEST TEST TEST !!!!
    string name = "faceTransition_w_breakdowns_anim";

    var sequenceWrapper = Newtonsoft.Json.JsonConvert.DeserializeObject<Dictionary<string, List<KeyFrame>>>(
        System.IO.File.ReadAllText(Application.dataPath + string.Format("/../../../lib/anki/products-cozmo-assets/animations/{0}.json", name)),
      new JsonSerializerSettings() {
        TypeNameHandling = TypeNameHandling.None,
        Converters = new List<JsonConverter> {
          new UtcDateTimeConverter(),
          new Newtonsoft.Json.Converters.StringEnumConverter()
        }
      });

    var sequence = sequenceWrapper[name];

    System.Action playAnimation = null;

    playAnimation = () => {
      PlayAnimation(sequence, playAnimation);
    };

    playAnimation();
  }

  private void Update() {
    UpdateSequence();
    UpdateMaterial();
  }

  public void PlayAnimation(List<KeyFrame> sequence, System.Action callback = null) {

    _CurrentSequence = sequence;
    _CurrentSequence.RemoveAll(x => x.Name != KeyFrameName.ProceduralFaceKeyFrame);
    _CurrentSequence.Sort((x, y) => {
      return x.triggerTime_ms.CompareTo(y.triggerTime_ms);
    });
    _CurrentSequenceIndex = 0;
    _CurrentSequenceStartTime = System.DateTime.UtcNow.ToUtcMs();
    _CurrentSequenceCompleteCallback = callback;
  }

  private void UpdateSequence() {
    if (_CurrentSequence == null) {
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
    }
    else {
      // Reached End of animation
      _CurrentSequence = null;
      var callback = _CurrentSequenceCompleteCallback;
      _CurrentSequenceCompleteCallback = null;
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
