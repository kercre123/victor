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
    public class LightningBoltPathScript : LightningBoltPrefabScriptBase
    {
        [Tooltip("How fast the lightning moves through the points or objects. 1 is normal speed, " +
            "0.01 is slower, so the lightning will move slowly between the points or objects.")]
        [Range(0.01f, 1.0f)]
        public float Speed = 1.0f;

        [SingleLineClamp("When each new point is moved to, this can provide a random value to make the movement to " +
            "the next point appear more staggered or random. Leave as 1 and 1 to have constant speed. Use a higher " +
            "maximum to create more randomness.", 1.0, 500.0)]
        public RangeOfFloats SpeedIntervalRange = new RangeOfFloats { Minimum = 1.0f, Maximum = 1.0f };

        [Tooltip("Repeat when the path completes?")]
        public bool Repeat = true;

        [ReorderableListAttribute("The game objects to follow for the lightning path")]
        public ReorderableList_GameObject LightningPath;
        private List<GameObject> lightningPath;

        private float nextInterval = 1.0f;
        private int nextIndex;
        private Vector3? lastPoint;

        protected override void Start()
        {
            base.Start();

            lightningPath = (LightningPath == null ? null : LightningPath.List);
        }

#if UNITY_EDITOR

        private readonly List<GameObject> lastGizmos = new List<GameObject>();

        protected override void OnDrawGizmos()
        {
            base.OnDrawGizmos();

            bool noLightningPath = (LightningPath == null || LightningPath.List == null || LightningPath.List.Count == 0);

            // remove any objects that were taken out of the list and cleanup the gizmo script
            for (int i = lastGizmos.Count - 1; i >= 0; i--)
            {
                if (noLightningPath || !LightningPath.List.Contains(lastGizmos[i]))
                {
                    if (lastGizmos[i] != null)
                    {
                        GameObject.DestroyImmediate(lastGizmos[i].GetComponent<LightningGizmoScript>());
                    }
                    lastGizmos.RemoveAt(i);
                }
            }

            // no objects, we are done
            if (noLightningPath)
            {
                return;
            }

            // add gizmo scripts and draw lines as needed
            Vector3 gizmoPosition;
            HashSet<GameObject> gizmos = new HashSet<GameObject>();
            Vector3? previousPoint = null;
            LightningGizmoScript gizmoScript;
            lastGizmos.Clear();

            for (int index = 0; index < LightningPath.List.Count; index++)
            {
                GameObject o = LightningPath.List[index];
                if (o == null)
                {
                    LightningPath.List.RemoveAt(index--);
                    continue;
                }
                else if ((gizmoScript = o.GetComponent<LightningGizmoScript>()) == null)
                {
                    // we need to add the gizmo script so that this object can be selectable by tapping on the lightning bolt in the scene view
                    gizmoScript = o.AddComponent<LightningGizmoScript>();
                    gizmoScript.hideFlags = HideFlags.DontSaveInBuild | HideFlags.HideInInspector | HideFlags.NotEditable;
                }

                // setup label based on whether we've seen this one before
                if (gizmos.Add(o))
                {
                    gizmoScript.Label = index.ToString();
                }
                else
                {
                    gizmoScript.Label += ", " + index.ToString();
                }

                gizmoPosition = o.transform.position;
                if (previousPoint != null)
                {
                    // draw a line and arrow in the proper direction
                    Gizmos.DrawLine(previousPoint.Value, gizmoPosition);
                    Vector3 direction = (gizmoPosition - previousPoint.Value);
                    Vector3 center = (previousPoint.Value + gizmoPosition) * 0.5f;
                    float arrowSize = Mathf.Min(1.0f, direction.magnitude);
                    UnityEditor.Handles.ArrowCap(0, center, Quaternion.LookRotation(direction), arrowSize);
                }

                previousPoint = gizmoPosition;
                lastGizmos.Add(o);
            }
        }

#endif

        public override void BatchLightningBolt(LightningBoltParameters parameters)
        {
            Vector3? currentPoint = null;

            if (lightningPath == null || lightningPath.Count < 2)
            {
                return;
            }
            else if (nextIndex >= lightningPath.Count)
            {
                if (!Repeat)
                {
                    return;
                }
                else if (lightningPath[lightningPath.Count - 1] == lightningPath[0])
                {
                    nextIndex = 1;
                }
                else
                {
                    nextIndex = 0;
                    lastPoint = null;
                }
            }
            try
            {
                if (lastPoint == null)
                {
                    lastPoint = lightningPath[nextIndex++].transform.position;
                }
                currentPoint = lightningPath[nextIndex].transform.position;
                if (lastPoint != null && currentPoint != null)
                {
                    parameters.Start = lastPoint.Value;
                    parameters.End = currentPoint.Value;

                    base.BatchLightningBolt(parameters);

                    if ((nextInterval -= Speed) <= 0.0f)
                    {
                        float speedValue = UnityEngine.Random.Range(SpeedIntervalRange.Minimum, SpeedIntervalRange.Maximum);
                        nextInterval = speedValue + nextInterval;
                        lastPoint = currentPoint;
                        nextIndex++;
                    }
                }
            }
            catch (System.NullReferenceException)
            {
                // null reference exception can happen here, in which case we bail as the list is broken until the null object gets taken out
            }
        }

        public void Reset()
        {
            lastPoint = null;
            nextIndex = 0;
            nextInterval = 1.0f;
        }
    }
}
