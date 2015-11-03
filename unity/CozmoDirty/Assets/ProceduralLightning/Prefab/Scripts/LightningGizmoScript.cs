using UnityEngine;
using System.Collections;

#if UNITY_EDITOR

namespace DigitalRuby.ThunderAndLightning
{
    public class LightningGizmoScript : MonoBehaviour
    {
        public string Label = string.Empty;

        private static readonly Vector3 labelOffset = Vector3.up * 1.5f;
        private static GUIStyle style;

        private void OnDrawGizmos()
        {
            if (style == null)
            {
                style = new GUIStyle();
                style.fontSize = 14;
                style.fontStyle = FontStyle.Normal;
                style.normal.textColor = Color.white;
            }
            Vector3 v = gameObject.transform.position;
            if (Label == "0" || Label.StartsWith("0,"))
            {
                Gizmos.DrawIcon(v, "LightningPathStart.png");
            }
            else
            {
                Gizmos.DrawIcon(v, "LightningPathNext.png");
            }
            UnityEditor.Handles.Label(v + labelOffset, Label, style);
        }
    }
}

#endif
