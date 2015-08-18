//
// Procedural Lightning for Unity
// (c) 2015 Digital Ruby, LLC
// Source code may be used for personal or commercial projects.
// Source code may NOT be redistributed or sold.
// 

using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace DigitalRuby.ThunderAndLightning
{
    public abstract class LightningBoltPrefabScriptBase : LightningBoltScript
    {
        [SingleLineClamp("How long to wait before creating another round of lightning bolts in seconds", 0.001, double.MaxValue)]
        public RangeOfFloats IntervalRange = new RangeOfFloats { Minimum = 0.05f, Maximum = 0.1f };

        [SingleLineClamp("How many lightning bolts to emit for each interval", 0.0, 100.0)]
        public RangeOfIntegers CountRange = new RangeOfIntegers { Minimum = 1, Maximum = 1 };

        [SingleLineClamp("For each bolt emitted, how long should it stay in seconds", 0.01, 10.0)]
        public RangeOfFloats DurationRange = new RangeOfFloats { Minimum = 0.06f, Maximum = 0.12f };

        [SingleLineClamp("The trunk width range in unity units (x = min, y = max)", 0.0001, 100.0)]
        public RangeOfFloats TrunkWidthRange = new RangeOfFloats { Minimum = 0.1f, Maximum = 0.2f };

        [Tooltip("How long (in seconds) this game object should live before destroying itself. Leave as 0 for infinite.")]
        [Range(0.0f, 1000.0f)]
        public float LifeTime = 0.0f;

        [Tooltip("Generations (1 - 8, higher makes more detailed but more expensive lightning)")]
        [Range(1, 8)]
        public int Generations = 6;

        [Tooltip("The chaos factor determines how far the lightning can spread out, higher numbers spread out more. 0 - 1.")]
        [Range(0.0f, 1.0f)]
        public float ChaosFactor = 0.075f;

        [Tooltip("The intensity of the glow, 0 - 1")]
        [Range(0.0f, 1.0f)]
        public float GlowIntensity = 0.1f;

        [Tooltip("The width multiplier for the glow, 0 - 64")]
        [Range(0.0f, 64.0f)]
        public float GlowWidthMultiplier = 4.0f;

        [Tooltip("How forked should the lightning be? (0 - 1, 0 for none, 1 for lots of forks)")]
        [Range(0.0f, 1.0f)]
        public float Forkedness = 0.25f;

        [Tooltip("What percent of time the lightning should fade in and out. For example, 0.15 fades in 15% of the time and fades out 15% of the time, with full visibility 70% of the time.")]
        [Range(0.0f, 0.5f)]
        public float FadePercent = 0.15f;

        [Tooltip("0 - 1, how slowly the lightning should grow. 0 for instant, 1 for slow.")]
        [Range(0.0f, 1.0f)]
        public float GrowthMultiplier;

        [Tooltip("How much smaller the lightning should get as it goes towards the end of the bolt. For example, 0.5 will make the end 50% the width of the start.")]
        [Range(0.0f, 10.0f)]
        public float EndWidthMultiplier = 0.5f;

        private readonly List<LightningBoltParameters> batchParameters = new List<LightningBoltParameters>();

        private static readonly System.Random random = new System.Random();
        private const float duration = 0.3f;
        private const float overlapMultiplier = 0.35f;
        private float nextArc;
        private float lifeTimeRemaining;

        private void CreateInterval(float offset)
        {
            nextArc = offset + ((float)random.NextDouble() * (IntervalRange.Maximum - IntervalRange.Minimum)) + IntervalRange.Minimum;
        }

        private void CallLightning()
        {
            int count = random.Next(CountRange.Minimum, CountRange.Maximum + 1);
            while (count-- > 0)
            {

        SingleLightningBolt();
            }

            CreateLightningBolts(batchParameters);
            batchParameters.Clear();
        }

    public void SingleLightningBolt() {
      float duration = ((float)random.NextDouble() * (DurationRange.Maximum - DurationRange.Minimum)) + DurationRange.Maximum;
      float trunkWidth = ((float)random.NextDouble() * (TrunkWidthRange.Maximum - TrunkWidthRange.Minimum)) + TrunkWidthRange.Maximum;
      
      LightningBoltParameters parameters = new LightningBoltParameters
      {
        Generations = Generations,
        LifeTime = duration,
        ChaosFactor = ChaosFactor,
        PointLightIntensity = 0.0f,
        TrunkWidth = trunkWidth,
        GlowIntensity = GlowIntensity,
        GlowWidthMultiplier = GlowWidthMultiplier,
        Forkedness = Forkedness,
        FadePercent = FadePercent,
        GrowthMultiplier = GrowthMultiplier,
        EndWidthMultiplier = EndWidthMultiplier,
        Random = random
      };
      BatchLightningBolt(parameters);
    }
  
        protected override void Start()
        {
            base.Start();
            CreateInterval(0.0f);
            lifeTimeRemaining = (LifeTime <= 0.0f ? float.MaxValue : LifeTime);
        }

        protected override void Update()
        {
            base.Update();

            if ((lifeTimeRemaining -= Time.deltaTime) < 0.0f)
            {
                GameObject.Destroy(gameObject);
            }
            else if ((nextArc -= Time.deltaTime) < 0)
            {
                CreateInterval(nextArc);
                CallLightning();
            }
        }

#if UNITY_EDITOR

        protected virtual void OnDrawGizmos()
        {
            Gizmos.color = Color.white;
            UnityEditor.Handles.color = Color.white;
        }

#endif

        public override void BatchLightningBolt(LightningBoltParameters p)
        {
            batchParameters.Add(p);

            // do not call the base method, we batch up and use CreateLightningBolts
        }

    }

    public class LightningBoltPrefabScript : LightningBoltPrefabScriptBase
    {
        [Tooltip("The source game object")]
        public GameObject Source;

        [Tooltip("The destination game object")]
        public GameObject Destination;

#if UNITY_EDITOR

        protected override void OnDrawGizmos()
        {
            base.OnDrawGizmos();

            if (Source != null)
            {
                Gizmos.DrawIcon(Source.transform.position, "LightningPathStart.png");
            }
            if (Destination != null)
            {
                Gizmos.DrawIcon(Destination.transform.position, "LightningPathNext.png");
            }
            if (Source != null && Destination != null)
            {
                Gizmos.DrawLine(Source.transform.position, Destination.transform.position);
                Vector3 direction = (Destination.transform.position - Source.transform.position);
                Vector3 center = (Source.transform.position + Destination.transform.position) * 0.5f;
                float arrowSize = Mathf.Min(2.0f, direction.magnitude);
                UnityEditor.Handles.ArrowCap(0, center, Quaternion.LookRotation(direction), arrowSize);
            }
        }

#endif

        public override void BatchLightningBolt(LightningBoltParameters parameters)
        {
            if (Source == null || Destination == null)
            {
                return;
            }
            parameters.Start = Source.transform.position;
            parameters.End = Destination.transform.position;

            base.BatchLightningBolt(parameters);
        }
    }
}
