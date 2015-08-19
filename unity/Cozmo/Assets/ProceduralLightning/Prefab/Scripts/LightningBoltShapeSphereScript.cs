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
    public class LightningBoltShapeSphereScript : LightningBoltPrefabScriptBase
    {
        [Tooltip("Radius inside the sphere where lightning can emit from")]
        public float InnerRadius = 0.1f;

        [Tooltip("Radius of the sphere")]
        public float Radius = 4.0f;

    public bool flattenToCircle = false;

#if UNITY_EDITOR

        protected override void OnDrawGizmos()
        {
            base.OnDrawGizmos();

            Gizmos.DrawWireSphere(transform.position, InnerRadius);
            Gizmos.DrawWireSphere(transform.position, Radius);
        }

#endif

        public override void BatchLightningBolt(LightningBoltParameters parameters)
        {
      //Debug.Log("frame("+Time.frameCount+") BatchLightningBolt!");
            Vector3 start = UnityEngine.Random.insideUnitSphere * InnerRadius;
            Vector3 end = UnityEngine.Random.onUnitSphere * Radius;

      if(flattenToCircle) {
        start = UnityEngine.Random.insideUnitCircle * InnerRadius;
        end = UnityEngine.Random.insideUnitCircle.normalized * Radius;  
      }
      else {
        start = UnityEngine.Random.insideUnitSphere * InnerRadius;
        end = UnityEngine.Random.onUnitSphere * Radius;  
      }

            parameters.Start = start;
            parameters.End = end;

            base.BatchLightningBolt(parameters);
        }
    }
}