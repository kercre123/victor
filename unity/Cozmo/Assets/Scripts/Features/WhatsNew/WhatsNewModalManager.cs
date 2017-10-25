using Anki.Assets;
using Cozmo.UI;
using DataPersistence;
using System.Collections.Generic;
using UnityEngine;

namespace Cozmo.WhatsNew {
  public class WhatsNewModalManager : MonoBehaviour {
    public delegate void CodeLabConnectPressedHandler();
    public static event CodeLabConnectPressedHandler OnCodeLabConnectPressed;

    private static System.Guid _sAutoOpenCodeLabProjectGuid;
    public static System.Guid AutoOpenCodeLabProjectGuid {
      get { return _sAutoOpenCodeLabProjectGuid; }
      set { _sAutoOpenCodeLabProjectGuid = value; }
    }
    public static bool ShouldAutoOpenProject {
      get { return _sAutoOpenCodeLabProjectGuid != System.Guid.Empty; }
    }

    private static WhatsNewDataCheck _sCurrentWhatsNewDataCheck = null;
    public static DataPersistence.Date CurrentWhatsNewDataCheckDate {
      get { return (_sCurrentWhatsNewDataCheck != null) ? _sCurrentWhatsNewDataCheck.DateChecked : new Date(); }
    }
    public static string CurrentWhatsNewDataID {
      get { return (_sCurrentWhatsNewDataCheck != null) ? _sCurrentWhatsNewDataCheck.DataFound.FeatureID : null; }
    }

    [SerializeField]
    private GameObjectDataLink _WhatsNewModalPrefabData;
    private WhatsNewModal _WhatsNewModalInstance;

    [SerializeField]
    private TextAsset _WhatsNewDataFile;
    private List<WhatsNewData> _WhatsNewDataList;

    private DataPersistence.Date? _OptedOutWhatsNewDate = null;

    private bool _IsOpeningWhatsNewModal = false;
    private bool _OpenWhatsNewModalAllowed = false;

    private void Start() {
      _OpenWhatsNewModalAllowed = false;
      _WhatsNewModalInstance = null;
      _sAutoOpenCodeLabProjectGuid = System.Guid.Empty;
      _sCurrentWhatsNewDataCheck = null;

      LoadWhatsNewData();

      BaseView.BaseViewOpenAnimationFinished += HandleViewOpenAnimationFinished;
      BaseView.BaseViewClosed += HandleViewClosed;
    }

    private void OnDestroy() {
      BaseView.BaseViewOpenAnimationFinished -= HandleViewOpenAnimationFinished;
      BaseView.BaseViewClosed -= HandleViewClosed;
    }

    private void LoadWhatsNewData() {
      string json = _WhatsNewDataFile.text;
      _WhatsNewDataList = null;

      try {
        _WhatsNewDataList = Newtonsoft.Json.JsonConvert.DeserializeObject<List<WhatsNewData>>(json);
      }
      catch (System.Exception e) {
        DAS.Error("WhatsNewModalManager.LoadWhatsNewData.JsonFileCorrupt", "Could not load whatsNew json file: " + e.Message);
      }

      // Sort from earliest start date to latest
      _WhatsNewDataList.Sort(WhatsNewData.CompareDataByStartDate);
    }

    private void OnApplicationPause(bool isPaused) {
      if (!isPaused && _OpenWhatsNewModalAllowed) {
        TryOpenWhatsNewModal();
      }
    }

    private void HandleViewOpenAnimationFinished(BaseView view) {
      if (view is Cozmo.ConnectionFlow.UI.NeedsUnconnectedView) {
        _OpenWhatsNewModalAllowed = true;
        TryOpenWhatsNewModal();
      }
    }

    public void HandleViewClosed(BaseView view) {
      _OpenWhatsNewModalAllowed = false;
    }

    private void TryOpenWhatsNewModal() {
#if ANKI_DEV_CHEATS
      if (_WhatsNewDataList == null) {
        DAS.Error("WhatsNewModalManager.TryOpenWhatsNewModal.JsonFileCorrupt", "whatsNew json file was not loaded on startup");
      }
#endif

      // Get date for today
      DataPersistence.Date today = DataPersistenceManager.Today;

      // Skip showing the whats new modal...
      if (_WhatsNewDataList == null                                                     // ... if we have no data
          || OnboardingManager.Instance.IsAnyOnboardingRequired()                       // ... if we are in onboarding
          || _OptedOutWhatsNewDate.HasValue && (_OptedOutWhatsNewDate.Value == today)   // ... if we have opted out today
          || _WhatsNewModalInstance != null                                             // ... if the modal is open
          || _IsOpeningWhatsNewModal) {                                                 // ... if the modal is loading
        return;
      }

      // ... if we don't have any sessions whatsoever
      List<TimelineEntryData> totalSessions = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions;
      if (totalSessions.Count <= 0) {
        return;
      }

      // ... if we have already connected today
      if (totalSessions[totalSessions.Count - 1].Date == today) {
        return;
      }

      // Update current whats new data to show if we haven't done it before or the date has changed.
      if (_sCurrentWhatsNewDataCheck == null || _sCurrentWhatsNewDataCheck.DateChecked != today) {
        _sCurrentWhatsNewDataCheck = CheckCurrentWhatsNewData(today);
      }

      // Show whats new modal if there is anything to show
      if (_sCurrentWhatsNewDataCheck != null) {
        ShowWhatsNewModal();
      }
    }

    private WhatsNewDataCheck CheckCurrentWhatsNewData(Date today) {
      // Walk backwards through list, from latest to earliest
      for (int i = _WhatsNewDataList.Count - 1; i >= 0; i--) {

        // If end date exists and today is after the end date, continue
        if (_WhatsNewDataList[i].EndDate != null) {
          DataPersistence.Date endDate = SimpleDate.DateFromSimpleDate(_WhatsNewDataList[i].EndDate);
          if (today > endDate) {
            continue;
          }
        }

        // If today is before start date, continue
        DataPersistence.Date startDate = SimpleDate.DateFromSimpleDate(_WhatsNewDataList[i].StartDate);
        if (today < startDate) {
          continue;
        }

        // If we haven't seen this feature before, then show it; otherwise continue falling back
        // to older features.
        List<string> seenWhatsNewFeatures = DataPersistenceManager.Instance.Data.DefaultProfile.WhatsNewFeaturesSeen;
        if (!seenWhatsNewFeatures.Contains(_WhatsNewDataList[i].FeatureID)) {
          return new WhatsNewDataCheck(today, _WhatsNewDataList[i]);
        }
      }
      return null;
    }

    private void ShowWhatsNewModal() {
      _IsOpeningWhatsNewModal = true;
      AssetBundleManager.Instance.LoadAssetBundleAsync(_WhatsNewModalPrefabData.AssetBundle, LoadWhatsNewModal);
    }

    private void LoadWhatsNewModal(bool assetBundleSuccess) {
      if (assetBundleSuccess && _OpenWhatsNewModalAllowed) {
        _WhatsNewModalPrefabData.LoadAssetData((GameObject whatsNewModalPrefab) => {
          if (_OpenWhatsNewModalAllowed && _WhatsNewModalInstance == null && whatsNewModalPrefab != null) {
            UIManager.OpenModal(whatsNewModalPrefab.GetComponent<WhatsNewModal>(),
                                new ModalPriorityData(ModalPriorityLayer.Low, 0,
                                                      LowPriorityModalAction.Queue, HighPriorityModalAction.Stack),
                                HandleWhatsNewModalCreated);
          }
        });
      }
      else {
        DAS.Error("NeedsConnectionManager.LoadFirstTimeView", "Failed to load asset bundle " + _WhatsNewModalPrefabData.AssetBundle);
      }
    }

    private void HandleWhatsNewModalCreated(BaseModal whatsNewModal) {
      _WhatsNewModalInstance = (WhatsNewModal)whatsNewModal;
      _WhatsNewModalInstance.InitializeWhatsNewModal(_sCurrentWhatsNewDataCheck.DataFound);
      _WhatsNewModalInstance.OnOptOutButtonPressed += HandleOptOutButtonPressed;
      _WhatsNewModalInstance.OnCodeLabButtonPressed += HandleCodeLabButtonPressed;
      _IsOpeningWhatsNewModal = false;
    }

    private void HandleOptOutButtonPressed() {
      // We're opting out of a particular whats new notification for a day.
      // Don't use DataPersistenceManager.Today because the date might have changed between showing
      // the dialog and pressing opt out
      _OptedOutWhatsNewDate = _sCurrentWhatsNewDataCheck.DateChecked;
    }

    private void HandleCodeLabButtonPressed(System.Guid projectGuid) {
      if (_sAutoOpenCodeLabProjectGuid == System.Guid.Empty) {
        _sAutoOpenCodeLabProjectGuid = projectGuid;
        if (OnCodeLabConnectPressed != null) {
          OnCodeLabConnectPressed();
        }
      }
    }
  }

  public class WhatsNewDataCheck {
    public DataPersistence.Date DateChecked { get; private set; }
    public WhatsNewData DataFound { get; private set; }

    public WhatsNewDataCheck(Date dateChecked, WhatsNewData dataFound) {
      DateChecked = dateChecked;
      DataFound = dataFound;
    }
  }

  public class WhatsNewData {
    public string FeatureID;
    public string DASEventFeatureID;

    public SimpleDate StartDate;
    public SimpleDate EndDate;

    public string TitleKey;
    public string DescriptionKey;
    public string IconAssetBundleName;
    public string IconAssetName;
    public int[] LeftTintColor;
    public int[] RightTintColor;
    public CodeLabData CodeLabData;

    public static int CompareDataByStartDate(WhatsNewData a, WhatsNewData b) {
      DataPersistence.Date dateA = SimpleDate.DateFromSimpleDate(a.StartDate);
      DataPersistence.Date dateB = SimpleDate.DateFromSimpleDate(b.StartDate);
      if (dateA < dateB) {
        return -1;
      }
      else if (dateA > dateB) {
        return 1;
      }
      return 0;
    }

    public static Color GetColor(int[] rgbValues) {
      Color tintColor = Color.white;
      if (rgbValues != null) {
        if (rgbValues.Length >= 1) {
          tintColor.r = rgbValues[0] / 255f;
        }
        if (rgbValues.Length >= 2) {
          tintColor.g = rgbValues[1] / 255f;
        }
        if (rgbValues.Length >= 3) {
          tintColor.b = rgbValues[2] / 255f;
        }
      }
      return tintColor;
    }

    public static float[] GetFloatArray(int[] rgbValues) {
      float[] tintColor = { 1f, 1f, 1f, 1f };
      if (rgbValues != null) {
        if (rgbValues.Length >= 1) {
          tintColor[0] = rgbValues[0] / 255f;
        }
        if (rgbValues.Length >= 2) {
          tintColor[1] = rgbValues[1] / 255f;
        }
        if (rgbValues.Length >= 3) {
          tintColor[2] = rgbValues[2] / 255f;
        }
      }
      return tintColor;
    }
  }

  public class CodeLabData {
    public bool ShowButton;
    public System.Guid ProjectGuid;
  }

  public class SimpleDate {
    public int Day;
    public int Month;
    public int Year;

    public static DataPersistence.Date DateFromSimpleDate(SimpleDate date) {
      return new DataPersistence.Date(date.Year, date.Month, date.Day);
    }

    public override string ToString() {
      return Month + "/" + Day + "/" + Year;
    }
  }
}