using System;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo {
  namespace UI {
    public class NeedsMeter : MonoBehaviour {

      #region Serialized Fields

      [SerializeField]
      private CozmoImage _ImageFillGlow = null;

      [SerializeField]
      private CozmoImage _ImageFillMask = null;

      [SerializeField]
      private CozmoImage[] _ImageEndCaps = null;

      [SerializeField]
      private CozmoImage _ImageBurst = null;

      [SerializeField]
      private ParticleSystem _ConstantParticles = null;

      [SerializeField]
      private ParticleSystem _StartBurstParticles = null;

      // 0 - 6 based on our quality level
      [SerializeField]
      private int[] _MinParticles = { 2, 5, 10, 15, 20, 20 };

      [SerializeField]
      private int[] _MaxParticles = { 10, 50, 80, 200, 300, 500 };

      [SerializeField]
      private float _MaskFillMin = 0f;

      [SerializeField]
      private float _MaskFillMax = 1f;

      [SerializeField]
      private float _GlowFillMin = 0f;

      [SerializeField]
      private float _GlowFillMax = 1f;

      [SerializeField]
      private float _GlowFillSnapValueIfAtMax = 1f;

      [SerializeField]
      private float _HideCapsUnderValue = 0.01f;

      [SerializeField]
      private float _HideCapsOverValue = 0.99f;

      [SerializeField]
      private float _FillTime_sec = 1f;

      [SerializeField]
      private float _BurstTime_sec = 1f;

      [SerializeField]
      private Color _OffColor = Color.clear;

      [SerializeField]
      private Color _OnColor = Color.white;

      [SerializeField]
      private AnimationCurve _BurstCurve = null;

      [SerializeField]
      private Color _NormalColor = Color.white;

      [SerializeField]
      private Color _DecreasingColor = Color.white;

      [SerializeField]
      private Color _IncreasingColor = Color.white;

      #endregion //Serialized Fields

      #region Non-serialized Fields

      private float _AnimStartValue = 0f;
      private float _TargetValue = 0f;

      private float _TotalFillPortion = 0f;
      private float TotalFillPortion {
        get {
          if (_TotalFillPortion <= 0f || Application.isEditor) {
            _TotalFillPortion = _MaskFillMax - _MaskFillMin;
          }
          return _TotalFillPortion;
        }
      }

      private float CurrentValue {
        get {
          float currentDisplayedPortion = _ImageFillMask.fillAmount - _MaskFillMin;
          float normalizedValue = 1f;
          if (TotalFillPortion > 0f) {
            normalizedValue = Mathf.Clamp01(currentDisplayedPortion / TotalFillPortion);
          }

          return normalizedValue;
        }
        set {
          float clampedValue = Mathf.Clamp01(value);
          float fillAmount = _MaskFillMin + (clampedValue * TotalFillPortion);
          fillAmount = Mathf.Clamp01(fillAmount);
          _ImageFillMask.fillAmount = fillAmount;

          float totalGlowPortion = _GlowFillMax - _GlowFillMin;
          float glowFillAmount = _GlowFillMin + (clampedValue * totalGlowPortion);

          if (glowFillAmount >= _GlowFillMax) {
            glowFillAmount = _GlowFillSnapValueIfAtMax;
          }

          _ImageFillGlow.fillAmount = glowFillAmount;
          //hide glow if bar empty
          _ImageFillGlow.gameObject.SetActive(clampedValue > 0f);
        }
      }

      private float _FillTimer = 0f;
      private float _BurstTimer = 0f;

      private bool _Initialized = false;

      #endregion // Non-serialized Fields

      #region Lifespan

      private void Start() {
        Initialize();
      }

      private void Initialize() {
        if (!_Initialized) {
          CurrentValue = 0f;
          SetBurstColor(_OffColor);
          if (_ImageEndCaps != null) {
            foreach (Image cap in _ImageEndCaps) {
              if (cap != null) {
                cap.gameObject.SetActive(false);
              }
            }
          }
          _Initialized = true;
        }
      }

      private void Update() {
        InterpolateFill(Time.deltaTime);
        InterpolateBurst(Time.deltaTime);
      }

      #endregion // LifeSpan

      #region Public Methods

      public void SetTargetAndAnimate(float target) {
        Initialize();
        SetTarget(target);
        _FillTimer = _FillTime_sec;
        _BurstTimer = 0f;
        _AnimStartValue = CurrentValue;
        _TargetValue = target;
        _StartBurstParticles.Play();
      }

      public void SetValueInstant(float target) {
        Initialize();
        SetValueInstantInternal(target);
      }

      #endregion // Public Methods

      #region Misc Private Helper Methods

      private float EaseOutQuad(float currentTimeSeconds, float startValue, float changeInValue, float duration) {
        currentTimeSeconds /= duration;
        return -changeInValue * currentTimeSeconds * (currentTimeSeconds - 2) + startValue;
      }

      private void SetValueInstantInternal(float target) {
        SetTarget(target);
        _AnimStartValue = _TargetValue;
        CurrentValue = _TargetValue;
        SetBurstColor(_OffColor);
        _FillTimer = 0f;
        _BurstTimer = 0f;
        PositionEndCaps();
      }

      private void SetBurstColor(Color col) {
        _ImageBurst.color = col;
        for (int i = 0; i < _ImageEndCaps.Length; i++) {
          CozmoImage cap = _ImageEndCaps[i];
          if (cap != null) {
            cap.color = col;
          }
        }
      }

      private void PositionEndCaps() {
        for (int i = 0; i < _ImageEndCaps.Length; i++) {
          CozmoImage cap = _ImageEndCaps[i];
          if (cap != null) {
            PositionEndCap(cap);
          }
        }
      }

      private void PositionEndCap(Image cap) {
        if (cap != null) {
          float current = CurrentValue;
          if (current < _HideCapsUnderValue || current > _HideCapsOverValue) {
            cap.gameObject.SetActive(false);
          }
          else {
            switch (_ImageFillMask.fillMethod) {
            case Image.FillMethod.Radial90:
            case Image.FillMethod.Radial180:
            case Image.FillMethod.Radial360:
              PositionRadialEndCap(cap);
              break;
            case Image.FillMethod.Vertical:
              PositionVerticalEndCap(cap);
              break;
            case Image.FillMethod.Horizontal:
              PositionHorizontalEndCap(cap);
              break;
            }
          }
        }
      }

      private void PositionRadialEndCap(Image cap) {
        float totalAngle = 360f;
        if (_ImageFillMask.fillMethod == Image.FillMethod.Radial90) {
          totalAngle = 90f;
        }
        else if (_ImageFillMask.fillMethod == Image.FillMethod.Radial180) {
          totalAngle = 180;
        }
        Vector3 eulers = cap.rectTransform.rotation.eulerAngles;
        eulers.z = _ImageFillMask.fillAmount * totalAngle * (_ImageFillMask.fillClockwise ? -1f : 1f);
        cap.rectTransform.eulerAngles = eulers;
        cap.gameObject.SetActive(true);
      }

      private void PositionHorizontalEndCap(Image cap) {
        float capPos = _ImageFillGlow.fillAmount * _ImageFillGlow.rectTransform.rect.width;
        if ((Image.OriginHorizontal)_ImageFillGlow.fillOrigin == Image.OriginHorizontal.Right) {
          capPos = -capPos;
        }
        cap.rectTransform.anchoredPosition = new Vector2(capPos, cap.rectTransform.anchoredPosition.y);
        cap.gameObject.SetActive(true);
      }

      private void PositionVerticalEndCap(Image cap) {
        float capPos = _ImageFillGlow.fillAmount * _ImageFillGlow.rectTransform.rect.height;
        if ((Image.OriginVertical)_ImageFillGlow.fillOrigin == Image.OriginVertical.Top) {
          capPos = -capPos;
        }
        cap.rectTransform.anchoredPosition = new Vector2(cap.rectTransform.anchoredPosition.x, capPos);
        cap.gameObject.SetActive(true);
      }

      private void InterpolateFill(float deltaTime) {
        if (_FillTimer > 0f) {
          _FillTimer -= deltaTime;
          if (_FillTimer < 0f) {
            _FillTimer = 0f;
          }

          CurrentValue = EaseOutQuad(_FillTime_sec - _FillTimer, _AnimStartValue,
            _TargetValue - _AnimStartValue, _FillTime_sec);

          PositionEndCaps();

          float currentValue = CurrentValue;

          SetBurstColor(_OffColor);

          var emissionMod = _ConstantParticles.emission;
          int lvl = PerformanceManager.Instance.GetQualitySetting();
          emissionMod.rateOverTime = Mathf.RoundToInt(Mathf.Lerp(_MinParticles[lvl], _MaxParticles[lvl], currentValue));

          if (_TargetValue > currentValue) {
            _ImageFillGlow.color = _IncreasingColor;
          }
          else if (_TargetValue < currentValue) {
            _ImageFillGlow.color = _DecreasingColor;
          }
          else {
            _ImageFillGlow.color = _NormalColor;
            _AnimStartValue = currentValue;
            _BurstTimer = _BurstTime_sec;
          }
        }
      }

      private void InterpolateBurst(float deltaTime) {
        if (_BurstTimer > 0f) {
          _BurstTimer -= deltaTime;
          if (_BurstTimer < 0f) {
            _BurstTimer = 0f;
          }

          float burstFactor = 0f;
          if (_BurstTime_sec > 0f) {
            burstFactor = 1f - Mathf.Clamp01(_BurstTimer / _BurstTime_sec);
          }
          SetBurstColor(Color.Lerp(_OffColor, _OnColor, _BurstCurve.Evaluate(burstFactor)));
        }
      }

      private void SetTarget(float target) {
        if (target < 0 || target > 1) {
          DAS.Warn("NeedsMeter.SetTarget.OutOfRange", "Tried to set progress to value="
             + target + " which is not in the range of 0 to 1! Clamping.");
        }
        _TargetValue = Mathf.Clamp01(target);
      }

      #endregion // Misc Private Helper Methods

    }
  }
}
