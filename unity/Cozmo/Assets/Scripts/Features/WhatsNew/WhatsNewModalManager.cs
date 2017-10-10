using Anki.Assets;
using Cozmo.UI;
using DataPersistence;
using System.Collections.Generic;
using UnityEngine;

namespace Cozmo.WhatsNew {
  public class WhatsNewModalManager : MonoBehaviour {
    public delegate void CodeLabConnectPressedHandler();
    public static event CodeLabConnectPressedHandler OnCodeLabConnectPressed;

    [SerializeField]
    private GameObjectDataLink _WhatsNewModalPrefabData;
    private WhatsNewModal _WhatsNewModalInstance;

    [SerializeField]
    private TextAsset _WhatsNewDataFile;
    private List<WhatsNewData> _WhatsNewDataList;
    private WhatsNewData _CurrentWhatsNewData;

    private bool _OpenWhatsNewModalAllowed = false;
    private bool _OptedOutWhatsNewThisAppRun;

    private static System.Guid _sAutoOpenCodeLabProjectGuid;
    public static System.Guid AutoOpenCodeLabProjectGuid {
      get { return _sAutoOpenCodeLabProjectGuid; }
      set { _sAutoOpenCodeLabProjectGuid = value; }
    }
    public static bool ShouldAutoOpenProject {
      get { return _sAutoOpenCodeLabProjectGuid != System.Guid.Empty; }
    }

    private void Start() {
      _OpenWhatsNewModalAllowed = false;
      _OptedOutWhatsNewThisAppRun = false;
      _WhatsNewModalInstance = null;
      _CurrentWhatsNewData = null;
      _sAutoOpenCodeLabProjectGuid = System.Guid.Empty;

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

      if (_WhatsNewDataList == null
          || _OptedOutWhatsNewThisAppRun
          || _WhatsNewModalInstance != null
          || _CurrentWhatsNewData != null
          || OnboardingManager.Instance.IsAnyOnboardingRequired()) {
        return;
      }

      List<TimelineEntryData> totalSessions = DataPersistenceManager.Instance.Data.DefaultProfile.Sessions;
      if (totalSessions.Count <= 0) {
        return;
      }

      TimelineEntryData lastSession = totalSessions[totalSessions.Count - 1];
      DataPersistence.Date lastConnectedDate = lastSession.Date;

      // Get date for today
      DataPersistence.Date today = DataPersistenceManager.Today;

      // COZMO-14671 9/21/2017 - Uncomment for easy debugging with current json data
      // today = new Date(2017, 12, 1);

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

        // If lastConnectedDate is after the start date, we've already seen this modal;
        // Assumed we have seen the rest of the list and break
        if (lastConnectedDate > startDate || lastConnectedDate == startDate) {
          break;
        }

        // Show the WhatsNewModal
        _CurrentWhatsNewData = _WhatsNewDataList[i];
        ShowWhatsNewModal();
        break;
      }
    }

    private void ShowWhatsNewModal() {
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
      _WhatsNewModalInstance.InitializeWhatsNewModal(_CurrentWhatsNewData);
      _WhatsNewModalInstance.OnOptOutButtonPressed += HandleOptOutButtonPressed;
      _WhatsNewModalInstance.OnCodeLabButtonPressed += HandleCodeLabButtonPressed;
      _CurrentWhatsNewData = null;
    }

    private void HandleOptOutButtonPressed() {
      _OptedOutWhatsNewThisAppRun = true;
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

  public class WhatsNewData {
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