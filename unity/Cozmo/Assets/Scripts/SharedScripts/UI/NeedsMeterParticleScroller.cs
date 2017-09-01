using UnityEngine;

namespace Cozmo {
  namespace UI {
    public class NeedsMeterParticleScroller : MonoBehaviour {
      [System.Serializable]
      public class ScrollData {
        public float ScrollSpeed_perSec = 0.005f;

        [Range(0f, 1f)]
        public float SpeedJitterMagnitude = 1f;
      }

      [SerializeField]
      private CozmoImage _ImageToScroll;

      [SerializeField]
      private Material _ScrollingMaterial;

      [SerializeField]
      private ScrollData _ParticleUScrollData;
      [SerializeField]
      private ScrollData _ParticleVScrollData;
      [SerializeField]
      private ScrollData _ParticleU2ScrollData;
      [SerializeField]
      private ScrollData _ParticleV2ScrollData;

      [SerializeField]
      private ScrollData _CloudMaskUScrollData;
      [SerializeField]
      private ScrollData _CloudMaskVScrollData;

      /// <summary>  
      /// We use one particle texture and scroll it in four directions.
      /// xy maps to the uv value of the first direction
      /// wz maps to the uv value for the second direction
      /// For the third and fourth directions, we flip this vector before passing it to the material.
      /// </summary>
      private Vector4 _CurrentParticleMoteAnimUVPair;

      /// <summary>
      /// Only xy is used, to track the uv value of the masking component to flicker 
      /// the particles in and out.
      /// </summary>
      private Vector4 _CurrentCloudMaskAnimUV;

      private float _CutoffValue = 0f;

      public float FillValue {
        set {
          _CutoffValue = value;
        }
      }

      private void Start() {
        // Create a copy of the material to avoid logging changes to the main asset every time we run the game.
        _ImageToScroll.material = new Material(_ScrollingMaterial);
        InitializeAnimationVariables();
      }

      private void Update() {
        ScrollImage(Time.deltaTime);
      }

      private void InitializeAnimationVariables() {
        // Offset the UVs so that the particles don't start in the exact same spot.
        _CurrentParticleMoteAnimUVPair = new Vector4(0f, 0f, 0.25f, 0.25f);
        _CurrentCloudMaskAnimUV = new Vector4(0.5f, 0.5f, 0.75f, 0.75f);
      }

      // Update is called once per frame
      private void ScrollImage(float deltaTime) {
        // Calculate the UV values for the first and second set of particles
        _CurrentParticleMoteAnimUVPair.x = AnimateUVs(_CurrentParticleMoteAnimUVPair.x, deltaTime, _ParticleUScrollData);
        _CurrentParticleMoteAnimUVPair.y = AnimateUVs(_CurrentParticleMoteAnimUVPair.y, deltaTime, _ParticleVScrollData);
        _CurrentParticleMoteAnimUVPair.z = AnimateUVs(_CurrentParticleMoteAnimUVPair.z, deltaTime, _ParticleU2ScrollData);
        _CurrentParticleMoteAnimUVPair.w = AnimateUVs(_CurrentParticleMoteAnimUVPair.w, deltaTime, _ParticleV2ScrollData);

        // Calulate the UV values for the moving mask
        _CurrentCloudMaskAnimUV.x = AnimateUVs(_CurrentCloudMaskAnimUV.x, deltaTime, _CloudMaskUScrollData);
        _CurrentCloudMaskAnimUV.y = AnimateUVs(_CurrentCloudMaskAnimUV.y, deltaTime, _CloudMaskVScrollData);

        Material imageMaterial = _ImageToScroll.material;
        if (imageMaterial != null) {
          imageMaterial.SetVector("_UVOffset", _CurrentParticleMoteAnimUVPair);
          // Flip the uv values for the third and fourth directions
          imageMaterial.SetVector("_UVOffset2", _CurrentParticleMoteAnimUVPair * -1f);
          imageMaterial.SetVector("_CloudUVOffset", _CurrentCloudMaskAnimUV);

          // Controls how much of the bar to show
          imageMaterial.SetFloat("_Cutoff", _CutoffValue);

          // Controls how much cloud masking to use
          imageMaterial.SetFloat("_CutoffSqr", _CutoffValue * _CutoffValue);
        }
      }

      private float AnimateUVs(float currentAnimUV, float deltaTime, ScrollData data) {
        // Move UV value
        float movement = (deltaTime * data.ScrollSpeed_perSec * _CutoffValue);
        // TODO: Use AnimationCurves to create a more noodle-y effect
        float randomOffset = (movement * Random.Range(-data.SpeedJitterMagnitude, data.SpeedJitterMagnitude));
        currentAnimUV += movement + randomOffset;

        // Wrap UVs from 0-1
        if (currentAnimUV > 1) {
          currentAnimUV -= 1;
        }
        else if (currentAnimUV < 0) {
          currentAnimUV = 1 + currentAnimUV;
        }
        return currentAnimUV;
      }
    }
  }
}