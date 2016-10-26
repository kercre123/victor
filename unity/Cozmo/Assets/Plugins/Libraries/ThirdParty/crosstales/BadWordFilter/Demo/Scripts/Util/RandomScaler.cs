using UnityEngine;

namespace Crosstales.BWF.Demo.Util
{
    /// <summary>Random scale changer.</summary>
    public class RandomScaler : MonoBehaviour
    {

        #region Variables

        public Vector3 ScaleMin = Vector3.zero;
        public Vector3 ScaleMax = Vector3.one;
        public bool Uniform = false;
        public Vector2 ChangeInterval = new Vector2(10, 45);
        private Transform tf;
        private Vector3 endScale;
        private float elapsedTime = 0f;
        private float changeTime = 0f;
        private Vector3 startScale;
        private float lerpTime = 0f;

        #endregion

        #region MonoBehaviour methods

        void Start()
        {
            tf = transform;

            elapsedTime = changeTime = Random.Range(ChangeInterval.x, ChangeInterval.y);

            startScale = tf.localScale;
        }

        void Update()
        {
            elapsedTime += Time.deltaTime;

            if (elapsedTime > changeTime)
            {
                if (Uniform)
                {
                    endScale.x = endScale.y = endScale.z = Random.Range(ScaleMin.x, Mathf.Abs(ScaleMax.x));
                }
                else
                {
                    endScale.x = Random.Range(ScaleMin.x, Mathf.Abs(ScaleMax.x));
                    endScale.y = Random.Range(ScaleMin.y, Mathf.Abs(ScaleMax.y));
                    endScale.z = Random.Range(ScaleMin.z, Mathf.Abs(ScaleMax.z));
                }

                changeTime = Random.Range(ChangeInterval.x, ChangeInterval.y);

                lerpTime = elapsedTime = 0f;
            }

            tf.localScale = Vector3.Lerp(startScale, endScale, lerpTime);

            if (lerpTime < 1f)
            {
                lerpTime += Time.deltaTime / (changeTime - 0.2f);
            }
            else
            {
                startScale = tf.localScale;
            }
        }

        #endregion
    }
}
// Copyright 2015-2016 www.crosstales.com