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
    public class DemoConfigureScript : MonoBehaviour
    {
        private const string scriptTemplate = @"// Important, make sure this script is assigned properly, or you will get null ref exceptions.
    DigitalRuby.ThunderAndLightning.LightningBoltScript script = gameObject.GetComponent<DigitalRuby.ThunderAndLightning.LightningBoltScript>();
    int count = {0};
    float duration = {1}f;
    float delay = 0.0f;
    int seed = {2};
    System.Random r = new System.Random(seed);
    Vector3 start = new Vector3({3}f, {4}f, {5}f);
    Vector3 end = new Vector3({6}f, {7}f, {8}f);
    int generations = {9};
    float chaosFactor = {10}f;
    float trunkWidth = {11}f;
    float glowIntensity = {12}f;
    float glowWidthMultiplier = {13}f;
    float forkedness = {14}f;
    float singleDuration = (duration / (float)count);
    float fadePercent = {15}f;
    float growthMultiplier = {16}f;

    while (count-- > 0)
    {{
        DigitalRuby.ThunderAndLightning.LightningBoltParameters parameters = new DigitalRuby.ThunderAndLightning.LightningBoltParameters
        {{
            Camera = Camera.main,
            Start = start,
            End = end,
            Generations = generations,
            LifeTime = (count == 1 ? singleDuration : (singleDuration * (((float)r.NextDouble() * 0.4f) + 0.8f))),
            Delay = delay,
            ChaosFactor = chaosFactor,
            PointLightIntensity = 0.0f,
            TrunkWidth = trunkWidth,
            GlowIntensity = glowIntensity,
            GlowWidthMultiplier = glowWidthMultiplier,
            Forkedness = forkedness,
            Random = r,
            FadePercent = fadePercent, // set to 0 to disable fade in / out
            GrowthMultiplier = growthMultiplier
        }};
        script.CreateLightningBolt(parameters);
        delay += (singleDuration * (((float)r.NextDouble() * 0.4f) + 0.3f));
    }}";

        private int lastSeed;
        private Vector3 lastStart;
        private Vector3 lastEnd;

        public LightningBoltScript LightningBoltScript;
        public ThunderAndLightningScript ThunderAndLightningScript;

        public UnityEngine.UI.Slider GenerationsSlider;
        public UnityEngine.UI.Slider BoltCountSlider;
        public UnityEngine.UI.Slider DurationSlider;
        public UnityEngine.UI.Slider ChaosSlider;
        public UnityEngine.UI.Slider TrunkWidthSlider;
        public UnityEngine.UI.Slider ForkednessSlider;
        public UnityEngine.UI.Slider GlowIntensitySlider;
        public UnityEngine.UI.Slider GlowWidthSlider;
        public UnityEngine.UI.Slider FadePercentSlider;
        public UnityEngine.UI.Slider GrowthMultiplierSlider;
        public UnityEngine.UI.Slider DistanceSlider;
        public UnityEngine.UI.Text GenerationsValueLabel;
        public UnityEngine.UI.Text BoltCountValueLabel;
        public UnityEngine.UI.Text DurationValueLabel;
        public UnityEngine.UI.Text ChaosValueLabel;
        public UnityEngine.UI.Text TrunkWidthValueLabel;
        public UnityEngine.UI.Text ForkednessValueLabel;
        public UnityEngine.UI.Text GlowIntensityValueLabel;
        public UnityEngine.UI.Text GlowWidthValueLabel;
        public UnityEngine.UI.Text FadePercentValueLabel;
        public UnityEngine.UI.Text GrowthMultiplierValueLabel;
        public UnityEngine.UI.Text DistanceValueLabel;
        public UnityEngine.UI.Text SeedLabel;
        public UnityEngine.UI.RawImage StartImage;
        public UnityEngine.UI.RawImage EndImage;
        public UnityEngine.UI.Button CopySeedButton;
        public UnityEngine.UI.InputField SeedInputField;
        public UnityEngine.UI.Text SpaceBarLabel;
        public UnityEngine.UI.Toggle OrthographicToggle;

        public void GenerationsSliderChanged(float value)
        {
            UpdateUI();
        }

        public void BoltCountSliderChanged(float value)
        {
            UpdateUI();
        }

        public void DurationSliderChanged(float value)
        {
            UpdateUI();
        }

        public void LengthSliderValueChanged(float value)
        {
            UpdateUI();
        }

        public void TrunkSliderValueChanged(float value)
        {
            UpdateUI();
        }

        public void GlowSliderValueChanged(float value)
        {
            UpdateUI();
        }

        public void FadePercentValueChanged(float value)
        {
            UpdateUI();
        }

        public void GrowthMultiplierValueChanged(float value)
        {
            UpdateUI();
        }

        public void DistanceValueChanged(float value)
        {
            UpdateUI();
        }

        public void StartLightningDrag()
        {
            StartImage.transform.position = Input.mousePosition;
        }

        public void EndLightningDrag()
        {
            EndImage.transform.position = Input.mousePosition;
        }

        public void CreateButtonClicked()
        {
            CallLightning();
        }

        public void OrthographicToggleClicked()
        {
            if (OrthographicToggle.isOn)
            {
                Camera.main.orthographic = true;
                Camera.main.orthographicSize = Camera.main.pixelHeight * 0.5f;
            }
            else
            {
                Camera.main.orthographic = false;
            }
        }

        public void CopyButtonClicked()
        {
            SeedInputField.text = lastSeed.ToString();
            TextEditor te = new TextEditor();
            string copyText = string.Format(scriptTemplate,
                BoltCountSlider.value,
                DurationSlider.value,
                SeedInputField.text,
                lastStart.x, lastStart.y, lastStart.z,
                lastEnd.x, lastEnd.y, lastEnd.z,                
                GenerationsSlider.value,
                ChaosSlider.value,
                TrunkWidthSlider.value,
                GlowIntensitySlider.value,
                GlowWidthSlider.value,
                ForkednessSlider.value,
                FadePercentSlider.value,
                GrowthMultiplierSlider.value
            );
            te.content = new GUIContent(copyText);
            te.SelectAll();
            te.Copy();
        }

        public void ClearButtonClicked()
        {
            SeedInputField.text = string.Empty;
        }

        private void UpdateUI()
        {
            GenerationsValueLabel.text = GenerationsSlider.value.ToString("0");
            BoltCountValueLabel.text = BoltCountSlider.value.ToString("0");
            DurationValueLabel.text = DurationSlider.value.ToString("0.00");
            ChaosValueLabel.text = ChaosSlider.value.ToString("0.00");
            TrunkWidthValueLabel.text = TrunkWidthSlider.value.ToString("0.00");
            ForkednessValueLabel.text = ForkednessSlider.value.ToString("0.00");
            GlowIntensityValueLabel.text = GlowIntensitySlider.value.ToString("0.00");
            GlowWidthValueLabel.text = GlowWidthSlider.value.ToString("0.00");
            FadePercentValueLabel.text = FadePercentSlider.value.ToString("0.00");
            GrowthMultiplierValueLabel.text = GrowthMultiplierSlider.value.ToString("0.00");
            DistanceValueLabel.text = DistanceSlider.value.ToString("0.00");
        }

        private void CallLightning()
        {
            if (SpaceBarLabel != null)
            {
                SpaceBarLabel.CrossFadeColor(new Color(0.0f, 0.0f, 0.0f, 0.0f), 1.0f, true, true);
                SpaceBarLabel = null;
            }

            UnityEngine.Profiler.BeginSample("CreateLightningBolt");
            System.Diagnostics.Stopwatch timer = System.Diagnostics.Stopwatch.StartNew();

            lastStart = StartImage.transform.position + (Camera.main.transform.forward * DistanceSlider.value);
            lastEnd = EndImage.transform.position + (Camera.main.transform.forward * DistanceSlider.value);
            lastStart = Camera.main.ScreenToWorldPoint(lastStart);
            lastEnd = Camera.main.ScreenToWorldPoint(lastEnd);

            int count = (int)BoltCountSlider.value;
            float duration = DurationSlider.value;
            float delay = 0.0f;
            float chaosFactor = ChaosSlider.value;
            float trunkWidth = TrunkWidthSlider.value;
            float forkedness = ForkednessSlider.value;
            if (!int.TryParse(SeedInputField.text, out lastSeed))
            {
                lastSeed = UnityEngine.Random.Range(int.MinValue, int.MaxValue);
            }
            System.Random r = new System.Random(lastSeed);
            float singleDuration = Mathf.Max(1.0f / 15.0f, (duration / (float)count));
            float fadePercent = FadePercentSlider.value;
            float growthMultiplier = GrowthMultiplierSlider.value;

            while (count-- > 0)
            {
                LightningBoltParameters parameters = new LightningBoltParameters
                {
                    Start = lastStart,
                    End = lastEnd,
                    Generations = (int)GenerationsSlider.value,
                    LifeTime = (count == 1 ? singleDuration : (singleDuration * (((float)r.NextDouble() * 0.4f) + 0.8f))),
                    Delay = delay,
                    ChaosFactor = chaosFactor,
                    PointLightIntensity = 0.0f,
                    TrunkWidth = trunkWidth,
                    GlowIntensity = GlowIntensitySlider.value,
                    GlowWidthMultiplier = GlowWidthSlider.value,
                    Forkedness = forkedness,
                    Random = r,
                    FadePercent = fadePercent,
                    GrowthMultiplier = growthMultiplier
                };
                LightningBoltScript.BatchLightningBolt(parameters);
                delay += (singleDuration * (((float)r.NextDouble() * 0.4f) + 0.3f));
            }

            timer.Stop();
            UnityEngine.Profiler.EndSample();

            UpdateStatusLabel(timer.Elapsed);
        }

        private void UpdateStatusLabel(System.TimeSpan time)
        {
            SeedLabel.text = "Time to create: " + time.TotalMilliseconds.ToString() + "ms" +
                System.Environment.NewLine + "Seed: " + lastSeed.ToString() +
                System.Environment.NewLine + "Start: " + lastStart.ToString() +
                System.Environment.NewLine + "End: " + lastEnd.ToString() +
                System.Environment.NewLine + System.Environment.NewLine +
                "Use Spacebar to create a bolt" + System.Environment.NewLine +
                "Drag circle and anchor" + System.Environment.NewLine +
                "Type in seed or clear for random";
        }

        private void Start()
        {
            UpdateUI();
            ThunderAndLightningScript.enabled = false;
            LightningBoltScript.LightningOriginParticleSystem = null;
            UpdateStatusLabel(System.TimeSpan.Zero);
        }

        private void Update()
        {
            if (!SeedInputField.isFocused && Input.GetKeyDown(KeyCode.Space))
            {
                CallLightning();
            }
        }
    }
}
