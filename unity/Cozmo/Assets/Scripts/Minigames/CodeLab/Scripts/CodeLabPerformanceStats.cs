using Newtonsoft.Json;
using DataPersistence;

namespace CodeLab {

#if ANKI_DEV_CHEATS
  public class PerformanceStats {
    private class PerformanceStatsOutput {
      public string latency;
      public string unity;
      public string engineFreq;
      public string engine;
      public string jsToCS;
      public string jsToCSSub;
      public string csToJS;
      public string running;
    }

    private PerformanceStatsOutput _Output = new PerformanceStatsOutput();

    private int _NumEvaluateJSCallsThisTick = 0;
    private int _NumWebViewCallsThisTick = 0;
    private int _NumWebViewSubMessagesThisTick = 0; // there can be multiple requests per call!
    private float _AvgEvaluateJSCallsPerTick = 0.0f;
    private float _AvgWebViewCallsThisTick = 0.0f;
    private float _AvgWebViewSubMessagesThisTick = 0.0f;

    private int _NumTicks = 0;

    public void AddWebViewCall() {
      ++_NumWebViewCallsThisTick;
    }

    public void AddWebViewSubMessages(int numMessages) {
      _NumWebViewSubMessagesThisTick += numMessages;
    }

    public void AddEvaluateJSCall() {
      ++_NumEvaluateJSCallsThisTick;
    }

    private float SmoothedAverage(float oldValue, float newValue) {
      // New samples only have a small change to the average to give a smoothed moving average
      const float kSmoothRatio = 0.025f; // Smaller values == Smoother; 1.0 == no smoothing
      return oldValue + ((newValue - oldValue) * kSmoothRatio);
    }

    public void Update(CodeLabGame game, bool isProgramRunning) {
      ++_NumTicks;

      _AvgEvaluateJSCallsPerTick = SmoothedAverage(_AvgEvaluateJSCallsPerTick, (float)_NumEvaluateJSCallsThisTick);
      _AvgWebViewCallsThisTick = SmoothedAverage(_AvgWebViewCallsThisTick, (float)_NumWebViewCallsThisTick);
      _AvgWebViewSubMessagesThisTick = SmoothedAverage(_AvgWebViewSubMessagesThisTick, (float)_NumWebViewSubMessagesThisTick);

      _NumEvaluateJSCallsThisTick = 0;
      _NumWebViewCallsThisTick = 0;
      _NumWebViewSubMessagesThisTick = 0;

      // To minimize skewing of the data from cost of sending this, only update every N ticks
      if (DataPersistenceManager.Instance.Data.DebugPrefs.DisplayPerfDataInCodeLab && ((_NumTicks % 5) == 0)) {
        var perfHUD = Cozmo.PerformanceManager.Instance.PerfHUD;
        _Output.latency = perfHUD.GetLatencySectionData();
        _Output.unity = perfHUD.GetUnitySectionData();
        _Output.engineFreq = perfHUD.GetEngineFreqSectionData();
        _Output.engine = perfHUD.GetEngineSectionData();
        _Output.jsToCS = _AvgWebViewCallsThisTick.ToString("F2");
        _Output.jsToCSSub = _AvgWebViewSubMessagesThisTick.ToString("F2");
        _Output.csToJS = _AvgEvaluateJSCallsPerTick.ToString("F2");
        _Output.running = isProgramRunning.ToString();

        string statsOutputAsJSON = JsonConvert.SerializeObject(_Output);
        game.EvaluateJS(@"window.setPerformanceData(" + statsOutputAsJSON + ");");
      }
    }
  }
#endif // ANKI_DEV_CHEATS

}
