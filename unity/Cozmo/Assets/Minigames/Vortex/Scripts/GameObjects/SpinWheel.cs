using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using DG.Tweening;

namespace Vortex {

  public class SpinWheel : MonoBehaviour {

    private const int kNumSlices = 8;
    [SerializeField]
    private UnityEngine.UI.Image _WheelBG;
    [SerializeField]
    private GameObject _PieSlicePrefab;

    private List<PieSlice> _PieSliceList;
    private Sequence _SpinSequence;

    void Awake() {
      // Create the pie slice children.
      _PieSliceList = new List<PieSlice>();
      float sliceArc = 360f / kNumSlices; 
      float fillAmount = 1f / kNumSlices;
      Color[] imageColors = { Color.white, Color.cyan };

      for (int i = 0; i < kNumSlices; ++i) {
        GameObject slice = GameObject.Instantiate(_PieSlicePrefab);
        slice.transform.SetParent(this.transform, false);

        Color imageColor = imageColors[i % imageColors.Length];
        PieSlice script_slice = slice.GetComponent<PieSlice>();
        //script_slice.Init(fillAmount, sliceArc * i, sliceArc * 0.5f, (i % 4) + 1, imageColor);
        script_slice.Init(fillAmount, sliceArc * i, sliceArc * 0.5f, i, imageColor);
        _PieSliceList.Add(script_slice);
      }
    }

    void OnDestroy() {
      // Safety check
      if (_SpinSequence != null) {
        _SpinSequence.Kill(false);
      }
    }

    public void StartSpin(TweenCallback callback = null) {
      // Grab from a config of some kind ( 520 - 1260 )
      int final_angle = Random.Range(VortexGame.kSpinDegreesMin, VortexGame.kSpinDegreesMax);
      float rotation_time = Random.Range(VortexGame.kSpinTimeMin, VortexGame.kSpinTimeMax);

      _SpinSequence = DOTween.Sequence();
      Tweener move_tween = transform.DOLocalRotate(new Vector3(0, 0, final_angle), rotation_time, RotateMode.LocalAxisAdd);
      move_tween.SetEase(Ease.InQuad);
      _SpinSequence.Append(move_tween);
      // slow down to allow more guessing, maybe for polish have a sound here? just let it nudge a bit to make guessing easier.
      final_angle = Random.Range(90, 270);
      rotation_time = Random.Range(1.0f, 2.0f);
      Tween slower_tween = transform.DOLocalRotate(new Vector3(0, 0, final_angle), rotation_time, RotateMode.LocalAxisAdd);
      slower_tween.SetEase(Ease.OutCubic);
      _SpinSequence.Append(slower_tween);

      if (callback != null) {
        _SpinSequence.OnComplete(callback);
      }
    }

    public int GetNumberOnSelectedSlice() {
      int slice_value = 0;
      float sliceArc = 360f / kNumSlices; 
      float max_angle = (sliceArc / 2.0f);
      float select_angle = 180.0f;

      // 0 degrees is on the right based on how the image fill of the piewheel is set up on the prefab
      // This selects from the top.
      foreach (PieSlice slice in _PieSliceList) {
        /*DAS.Info("PieSlice: " + slice.Number, " Euler: " + slice.transform.eulerAngles.z +
        " min: " + (slice.transform.eulerAngles.z - max_angle) +
        " max: " + (slice.transform.eulerAngles.z + max_angle));*/
        if (select_angle >= (slice.transform.eulerAngles.z - max_angle) && select_angle <= (slice.transform.eulerAngles.z + max_angle)) {
          slice_value = slice.Number;
          DAS.Info("VortexGame", "wheel landed on " + slice_value);
        }
      }
      return slice_value;
    }
  }
}
