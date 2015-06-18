//
// Procedural Lightning for Unity
// (c) 2015 Digital Ruby, LLC
// Source code may be used for personal or commercial projects.
// Source code may NOT be redistributed or sold.
// 

using UnityEngine;
using System.Collections;

namespace DigitalRuby.ThunderAndLightning
{
    public class ThunderAndLightningScript : MonoBehaviour
    {
        private class LightningBoltHandler
        {
            private ThunderAndLightningScript script;

            public LightningBoltHandler(ThunderAndLightningScript script)
            {
                this.script = script;
                CalculateNextLightningTime();
            }

            private void UpdateLighting()
            {
                if (script.lightningInProgress)
                {
                    return;
                }

                if (script.ModifySkyboxExposure)
                {
                    script.skyboxExposureStorm = 0.35f;

                    if (script.skyboxMaterial != null)
                    {
                        script.skyboxMaterial.SetFloat("_Exposure", script.skyboxExposureStorm);
                    }
                }

                CheckForLightning();
            }

            private void CalculateNextLightningTime()
            {
                script.nextLightningTime = Time.time + UnityEngine.Random.Range(script.LightningIntervalTimeRange.x, script.LightningIntervalTimeRange.y);
                script.lightningInProgress = false;

                if (script.ModifySkyboxExposure)
                {
                    script.skyboxMaterial.SetFloat("_Exposure", script.skyboxExposureStorm);
                }
            }

            private IEnumerator CalculateNextLightningLater(float duration)
            {
                yield return new WaitForSeconds(duration);

                CalculateNextLightningTime();
            }

            public IEnumerator ProcessLightning(bool intense, bool visible)
            {
                float sleepTime;
                AudioClip[] sounds;
                float intensity;
                script.lightningInProgress = true;

                if (intense)
                {
                    float percent = UnityEngine.Random.Range(0.0f, 1.0f);
                    intensity = Mathf.Lerp(2.0f, 8.0f, percent);
                    sleepTime = 5.0f / intensity;
                    script.lightningDuration =  UnityEngine.Random.Range(script.LightningShowTimeRange.x, script.LightningShowTimeRange.y);
                    sounds = script.ThunderSoundsIntense;
                }
                else
                {
                    float percent = UnityEngine.Random.Range(0.0f, 1.0f);
                    intensity = Mathf.Lerp(0.0f, 2.0f, percent);
                    sleepTime = 30.0f / intensity;
                    script.lightningDuration = UnityEngine.Random.Range(script.LightningShowTimeRange.x, script.LightningShowTimeRange.y);
                    sounds = script.ThunderSoundsNormal;
                }
                if (script.skyboxMaterial != null && script.ModifySkyboxExposure)
                {
                    script.skyboxMaterial.SetFloat("_Exposure", Mathf.Max(intensity * 0.5f, script.skyboxExposureStorm));
                }

                // determine number of lightning strikes
                int count;
                int num = UnityEngine.Random.Range(0, 100);
                if (num > 95)
                {
                    count = 3;
                }
                else if (num > 80)
                {
                    count = 2;
                }
                else
                {
                    count = 1;
                }

                // determine duration - for multiple strikes, add a little extra time
                float duration = (count == 1 ? script.lightningDuration : (script.lightningDuration / count) * UnityEngine.Random.Range(1.05f, 1.25f));

                // make sure the duration lasts for all the lightning strikes
                script.lightningDuration *= count;

                // perform the strike
                Strike(intense, script.Generations, duration, intensity, script.ChaosFactor, script.GlowIntensity, script.GlowWidthMultiplier, script.Forkedness, count, script.Camera, visible ? script.Camera : null);

                // calculate the next lightning strike
                script.StartCoroutine(CalculateNextLightningLater(duration));

                // thunder will play depending on intensity of lightning
                bool playThunder = (intensity >= 1.0f);
                //Debug.Log("Lightning intensity: " + intensity.ToString("0.00") + ", thunder delay: " +
                //          (playThunder ? sleepTime.ToString("0.00") : "No Thunder"));

                if (playThunder && sounds != null && sounds.Length != 0)
                {
                    // wait for a bit then play a thunder sound
                    yield return new WaitForSeconds(sleepTime);

                    AudioClip clip = null;
                    do
                    {
                        // pick a random thunder sound that wasn't the same as the last sound, unless there is only one sound, then we have no choice
                        clip = sounds[UnityEngine.Random.Range(0, sounds.Length - 1)];
                    }
                    while (sounds.Length > 1 && clip == script.lastThunderSound);

                    // set the last sound and play it
                    script.lastThunderSound = clip;
                    script.audioSourceThunder.PlayOneShot(clip, intensity * 0.5f);
                }
            }

            private void Strike(bool intense, int generations, float duration, float intensity, float chaosFactor, float glowIntensity, float glowWidth, float forkedness, int count, Camera camera, Camera visibleInCamera)
            {
                if (count < 1)
                {
                    return;
                }

                // find a point around the camera that is not too close
                System.Random r = new System.Random();
                const float minDistance = 500.0f;
                float minValue = (intense ? -1000.0f : -5000.0f);
                float maxValue = (intense ? 1000 : 5000.0f);
                float closestValue = (intense ? 500.0f : 2500.0f);
                float x = (UnityEngine.Random.Range(0, 2) == 0 ? UnityEngine.Random.Range(minValue, -closestValue) : UnityEngine.Random.Range(closestValue, maxValue));
                float y = 620.0f;
                float z = (UnityEngine.Random.Range(0, 2) == 0 ? UnityEngine.Random.Range(minValue, -closestValue) : UnityEngine.Random.Range(closestValue, maxValue));
                float delay = 0.0f;
                Vector3 start = script.Camera.transform.position;
                start.x += x;
                start.y = y;
                start.z += z;

                if (visibleInCamera != null)
                {
                    // try and make sure the strike is visible in the camera
                    Quaternion q = visibleInCamera.transform.rotation;
                    visibleInCamera.transform.rotation = Quaternion.Euler(0.0f, q.eulerAngles.y, 0.0f);
                    float screenX = UnityEngine.Random.Range(visibleInCamera.pixelWidth * 0.1f, visibleInCamera.pixelWidth * 0.9f);
                    float ScreenZ = UnityEngine.Random.Range(visibleInCamera.nearClipPlane + closestValue + closestValue, maxValue);
                    Vector3 point = visibleInCamera.ScreenToWorldPoint(new Vector3(screenX, 0.0f, ScreenZ));
                    start = point;
                    start.y = y;
                    visibleInCamera.transform.rotation = q;
                }

                while (count-- > 0)
                {
                    // for each strike, calculate the end position and perform the strike
                    Vector3 end = start;

                    x = UnityEngine.Random.Range(-100, 100.0f);

                    // 1 in 4 chance not to strike the ground
                    y = (UnityEngine.Random.Range(0, 4) == 0 ? UnityEngine.Random.Range(-1, 600.0f) : -1.0f);

                    z += UnityEngine.Random.Range(-100.0f, 100.0f);

                    end.x += x;
                    end.y = y;
                    end.z += z;

                    // make sure the bolt points away from the camera
                    end.x += (closestValue * camera.transform.forward.x);
                    end.z += (closestValue * camera.transform.forward.z);

                    while ((start - end).magnitude < minDistance)
                    {
                        end.x += (closestValue * camera.transform.forward.x);
                        end.z += (closestValue * camera.transform.forward.z);
                    }

                    if (script.LightningBoltScript != null)
                    {
                        if (UnityEngine.Random.value < script.CloudLightningChance)
                        {
                            // cloud only lightning
                            generations = 0;
                        }
                        LightningBoltParameters parameters = new LightningBoltParameters
                        {
                            Start = start,
                            End = end,
                            Generations = generations,
                            LifeTime = duration,
                            Delay = delay,
                            ChaosFactor = chaosFactor,
                            PointLightIntensity = intensity,
                            TrunkWidth = 8.0f,
                            EndWidthMultiplier = 0.25f,
                            GlowIntensity = glowIntensity,
                            GlowWidthMultiplier = glowWidth,
                            Forkedness = forkedness,
                            Random = r
                        };
                        script.LightningBoltScript.BatchLightningBolt(parameters);
                        delay += ((duration / count) * UnityEngine.Random.Range(0.2f, 0.5f));
                    }
                }
            }

            private void CheckForLightning()
            {
                // time for another strike?
                if (Time.time >= script.nextLightningTime)
                {
                    bool intense = UnityEngine.Random.value < script.LightningIntenseProbability;
                    script.StartCoroutine(ProcessLightning(intense, script.LightningAlwaysVisible));
                }
            }

            public void Update()
            {
                UpdateLighting();
            }
        }

        [Tooltip("Lightning bolt script - optional, leave null if you don't want lightning bolts")]
        public LightningBoltScript LightningBoltScript;

        [Tooltip("Camera where the lightning should be centered over. Defaults to main camera.")]
        public Camera Camera;

        [Tooltip("Random duration that the scene will light up - intense lightning extends this a little.")]
        public Vector2 LightningShowTimeRange = new Vector2(0.2f, 0.5f);

        [Tooltip("Random interval between strikes.")]
        public Vector2 LightningIntervalTimeRange = new Vector2(10.0f, 25.0f);

        [Tooltip("Probability (0-1) of an intense lightning bolt that hits really close. Intense lightning has increased brightness and louder thunder compared to normal lightning, and the thunder sounds plays a lot sooner.")]
        [Range(0.0f, 1.0f)]
        public float LightningIntenseProbability = 0.2f;

        [Tooltip("Sounds to play for normal thunder. One will be chosen at random for each lightning strike. Depending on intensity, some normal lightning may not play a thunder sound.")]
        public AudioClip[] ThunderSoundsNormal;

        [Tooltip("Sounds to play for intense thunder. One will be chosen at random for each lightning strike.")]
        public AudioClip[] ThunderSoundsIntense;

        [Tooltip("Whether lightning strikes should always try to be in the camera view")]
        public bool LightningAlwaysVisible = true;

        [Tooltip("The chance lightning will simply be in the clouds with no visible bolt")]
        [Range(0.0f, 1.0f)]
        public float CloudLightningChance = 0.5f;

        [Tooltip("Whether to modify the skybox exposure when lightning is created")]
        public bool ModifySkyboxExposure = false;

        [Tooltip("How much the lightning should glow, 0 for none 1 for full glow")]
        [Range(0.0f, 1.0f)]
        public float GlowIntensity = 0.1f;

        [Tooltip("How the glow width should be multiplied, 0 for none, 64 is max")]
        [Range(0.0f, 64.0f)]
        public float GlowWidthMultiplier = 4.0f;

        [Tooltip("How forked the lightning should be. 0 for none, 1 for LOTS of forks.")]
        [Range(0.0f, 1.0f)]
        public float Forkedness = 0.5f;

        [Tooltip("How chaotic is the lightning? Higher numbers make more chaotic lightning.")]
        [Range(0.0f, 1.0f)]
        public float ChaosFactor = 0.2f;

        [Tooltip("Number of generations. The higher the number, the more detailed the lightning is, but more expensive to create.")]
        [Range(4, 8)]
        public int Generations = 6;

        private float skyboxExposureOriginal;
        private float skyboxExposureStorm;
        private float nextLightningTime;
        private float lightningDuration;
        private bool lightningInProgress;
        private AudioSource audioSourceThunder;
        private LightningBoltHandler lightningBoltHandler;
        private Material skyboxMaterial;
        private AudioClip lastThunderSound;

        private void Start()
        {
            if (Camera == null)
            {
                Camera = Camera.main;
            }

            if (Camera.farClipPlane < 10000.0f)
            {
                Debug.LogWarning("Far clip plane should be 10000+ for best lightning effects");
            }

            if (RenderSettings.skybox != null)
            {
                skyboxMaterial = RenderSettings.skybox = new Material(RenderSettings.skybox);
            }

            skyboxExposureOriginal = skyboxExposureStorm = (skyboxMaterial == null ? 1.0f : skyboxMaterial.GetFloat("_Exposure"));
            audioSourceThunder = gameObject.AddComponent<AudioSource>();
            lightningBoltHandler = new LightningBoltHandler(this);
        }

        private void Update()
        {
            if (lightningBoltHandler != null)
            {
                lightningBoltHandler.Update();
            }
        }

        public void CallNormalLightning()
        {
            StartCoroutine(lightningBoltHandler.ProcessLightning(false, true));
        }

        public void CallIntenseLightning()
        {
            StartCoroutine(lightningBoltHandler.ProcessLightning(true, true));
        }

        public float SkyboxExposureOriginal
        {
            get { return skyboxExposureOriginal; }
        }
    }
}