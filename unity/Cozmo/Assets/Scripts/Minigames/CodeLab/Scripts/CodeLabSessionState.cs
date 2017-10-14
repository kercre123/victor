using UnityEngine;
using Anki.Cozmo;
using Anki.Cozmo.ExternalInterface;
using Newtonsoft.Json;
using DataPersistence;
using System;
using System.Collections.Generic;
using System.IO;
using Cozmo.UI;
using Cozmo.MinigameWidgets;


namespace CodeLab {

  public enum GrammarMode {
    None,
    Horizontal,
    Vertical
  }

  public enum AnimTrack {
    Wheels = 0,
    Lift,
    Head,
    // Special values:
    Count
  }

  public enum ActionType {
    Drive = 0,
    Head,
    Lift,
    Say,
    Anim,
    // Special values:
    Count,
    All
  }

  public enum AlignmentX {
    Left = 0,
    Center,
    Right
  }

  public enum AlignmentY {
    Top = 0,
    Center,
    Bottom
  }

  public class ProjectStats {
    public enum EventCategory {
      updated_user_project = 0,
      loaded_from_file,
    }

    private Guid _ProjectUUID = Guid.Empty;
    public int NumBlocks = 0;
    public int NumConnections = 0;
    public int NumGreenFlags = 0;
    public int NumRepeat = 0;
    public int NumForever = 0;
    public bool HasPendingChanges = false;

    private static int CountStringOccurences(string contents, string searchString) {
      // Count the number of unique times searchString occurs within contents
      // does not count overlapping strings, e.g. CountStringOccurences("bababa", "baba") == 1
      // (this is not needed for this use case, and would be slower to calculate)
      int firstChar = 0;
      int numMatches = 0;
      while (firstChar >= 0) {
        int nextMatch = contents.IndexOf(searchString, firstChar, StringComparison.Ordinal);
        if (nextMatch >= 0) {
          numMatches += 1;
          firstChar = nextMatch + 1;
        }
        else {
          firstChar = -1;
        }
      }
      return numMatches;
    }

    public void Update(CodeLabProject project) {
      var projectJSON = project.ProjectJSON;

      // Slightly hacky - instead of parsing the JSON, just look for the entries we're interested in
      int numBlocks = 0; //CountStringOccurences(projectJSON, "opcode"); // TODO Fix for JSON
      int numConnections = 0; //CountStringOccurences(projectJSON, "<next>"); // TODO Fix for JSON
      int numGreenFlags = CountStringOccurences(projectJSON, "event_whenflagclicked");
      int numRepeat = CountStringOccurences(projectJSON, "control_repeat");
      int numForever = CountStringOccurences(projectJSON, "control_forever");

      bool wasUpdated = (_ProjectUUID != project.ProjectUUID) ||
        (NumBlocks != numBlocks) ||
        (NumConnections != numConnections) ||
        (NumGreenFlags != numGreenFlags) ||
        (NumRepeat != numRepeat) ||
        (NumForever != numForever);

      _ProjectUUID = project.ProjectUUID;
      NumBlocks = numBlocks;
      NumConnections = numConnections;
      NumGreenFlags = numGreenFlags;
      NumRepeat = numRepeat;
      NumForever = numForever;

      if (wasUpdated) {
        // Keep track of changes, but only upload if/when the program is actually run
        HasPendingChanges = true;
      }
    }

    public void PostPendingChanges(EventCategory category) {
      if (HasPendingChanges) {
        SessionState.DAS_Event("robot.code_lab." + category.ToString() + ".blocks", NumBlocks.ToString(), DASUtil.FormatExtraData(NumConnections.ToString()));
        SessionState.DAS_Event("robot.code_lab." + category.ToString() + ".repeat", NumRepeat.ToString(), DASUtil.FormatExtraData(NumForever.ToString()));
        SessionState.DAS_Event("robot.code_lab." + category.ToString() + ".green_flags", NumGreenFlags.ToString());
        HasPendingChanges = false;
      }
    }
  }

  public class SessionState {

    public class ChallengesState {
      private System.DateTime _OpenDateTime;
      private System.DateTime _OpenSlideDateTime;
      private uint _CurrentSlide = 0;
      private uint _NumSlidesViewed = 0;
      private bool _IsOpen = false;

      public bool IsOpen() {
        return _IsOpen;
      }

      public void Open() {
        if (_IsOpen) {
          DAS.Error("Codelab.Challenges.Open.AlreadyOpen", "");
        }
        DAS_Event("robot.code_lab.start_challenge_ui", "");
        _IsOpen = true;
        _OpenDateTime = System.DateTime.UtcNow;
        _CurrentSlide = 0;
        _NumSlidesViewed = 0;
      }

      private void EndViewingSlide() {
        if (_CurrentSlide > 0) {
          double timeOnSlide_s = (System.DateTime.UtcNow - _OpenSlideDateTime).TotalSeconds;
          DAS_Event("robot.code_lab.end_challenge_slide_view", _CurrentSlide.ToString(), DASUtil.FormatExtraData(timeOnSlide_s.ToString()));
          if (timeOnSlide_s > 1.0) {
            _NumSlidesViewed += 1;
          }
        }
      }

      public void Close() {
        if (_IsOpen) {
          EndViewingSlide();
          double timeOpen_s = (System.DateTime.UtcNow - _OpenDateTime).TotalSeconds;
          DAS_Event("robot.code_lab.end_challenge_ui", _NumSlidesViewed.ToString(), DASUtil.FormatExtraData(timeOpen_s.ToString()));
          _IsOpen = false;
        }
        else {
          DAS.Error("Codelab.Challenges.Close.NotOpen", "");
        }
      }

      public void SetSlideNumber(uint slideNum) {
        if (_CurrentSlide != slideNum) {
          EndViewingSlide();
          _CurrentSlide = slideNum;
          _OpenSlideDateTime = System.DateTime.UtcNow;
          DAS_Event("robot.code_lab.start_challenge_slide_view", _CurrentSlide.ToString());
        }
      }
    } // End class ChallengesState

    public class TutorialState {
      private System.DateTime _OpenDateTime;
      private bool _IsOpen = false;

      public bool IsOpen() {
        return _IsOpen;
      }

      public void Open() {
        if (_IsOpen) {
          DAS.Error("CodeLab.Tutorial.Open.AlreadyOpen", "");
        }
        DAS_Event("robot.code_lab.start_tutorial_ui", "");
        _IsOpen = true;
        _OpenDateTime = System.DateTime.UtcNow;
      }

      public void Close() {
        if (_IsOpen) {
          double timeOpen_s = (System.DateTime.UtcNow - _OpenDateTime).TotalSeconds;
          DAS_Event("robot.code_lab.end_tutorial_ui", timeOpen_s.ToString());
          _IsOpen = false;
        }
        else {
          DAS.Error("CodeLab.Tutorial.Close.NotOpen", "");
        }
      }
    } // End class TutorialState

    public class ProgramState {
      private System.DateTime _StartDateTime;
      private int _NumBlocksExecuted = 0;
      private bool _IsProgramRunning = false;
      private bool _WasProgramAborted = false;

      // Program state vars 
      private const bool kDefaultShouldWaitForActions = true;
      private const byte kDefaultDrawColor = (byte)1; // monochrome 1=On, 0=Off/Erase
      private const float kDefaultDrawTextScale = 1.0f;

      private bool[] _IsAnimTrackEnabled = new bool[(int)AnimTrack.Count];
      private bool _ShouldWaitForActions = kDefaultShouldWaitForActions;
      private byte _DrawColor = kDefaultDrawColor;
      private float _DrawTextScale = kDefaultDrawTextScale;
      private AlignmentX _DrawTextAlignmentX;
      private AlignmentY _DrawTextAlignmentY;

      public byte GetDrawColor() {
        return _DrawColor;
      }

      public void SetDrawColor(byte newVal) {
        _DrawColor = newVal;
      }

      public float GetDrawTextScale() {
        return _DrawTextScale;
      }

      public void SetDrawTextScale(float newVal) {
        _DrawTextScale = newVal;
      }

      public AlignmentX GetDrawTextAlignmentX() {
        return _DrawTextAlignmentX;
      }

      public AlignmentY GetDrawTextAlignmentY() {
        return _DrawTextAlignmentY;
      }

      public void SetDrawTextAlignment(uint xAlignment, uint yAlignment) {
        _DrawTextAlignmentX = (AlignmentX)xAlignment;
        _DrawTextAlignmentY = (AlignmentY)yAlignment;
      }

      public void SetShouldWaitForActions(bool newVal) {
        _ShouldWaitForActions = newVal;
      }

      public bool ShouldWaitForActions() {
        return _ShouldWaitForActions;
      }

      public bool IsAnimTrackEnabled(AnimTrack animTrack) {
        int animTrackInt = (int)animTrack;
        if ((animTrackInt >= 0) && (animTrackInt < _IsAnimTrackEnabled.Length)) {
          return _IsAnimTrackEnabled[animTrackInt];
        }
        else {
          DAS.Error("CodeLab.IsAnimTrackEnabled.BadAnimTrack", "Unhandled type " + animTrackInt + " = " + animTrack);
          return false;
        }
      }

      public void SetIsAnimTrackEnabled(AnimTrack animTrack, bool newVal) {
        int animTrackInt = (int)animTrack;
        if ((animTrackInt >= 0) && (animTrackInt < _IsAnimTrackEnabled.Length)) {
          if (_IsAnimTrackEnabled[animTrackInt] != newVal) {
            DAS.Warn("CodeLab.SetIsAnimTrackEnabled.Changing", animTrack.ToString() + " from "
                     + _IsAnimTrackEnabled[animTrackInt] + " to " + newVal);
            _IsAnimTrackEnabled[animTrackInt] = newVal;
          }
        }
        else {
          DAS.Error("CodeLab.SetIsAnimTrackEnabled.BadAnimTrack", "Unhandled type " + animTrackInt + " = " + animTrack);
        }
      }

      public bool IsProgramRunning() {
        return _IsProgramRunning;
      }

      public void Reset() {
        _NumBlocksExecuted = 0;
        _IsProgramRunning = false;
        _WasProgramAborted = false;
        ResetProgramStateVars();
      }

      private void ResetProgramStateVars() {
        for (int i = 0; i < _IsAnimTrackEnabled.Length; ++i) {
          _IsAnimTrackEnabled[i] = true;
        }
        _ShouldWaitForActions = kDefaultShouldWaitForActions;
        _DrawColor = kDefaultDrawColor;
        _DrawTextScale = kDefaultDrawTextScale;
        _DrawTextAlignmentX = AlignmentX.Left;
        _DrawTextAlignmentY = AlignmentY.Top;
      }

      public void OnBlockStarted() {
        ++_NumBlocksExecuted;
      }

      public void StartProgram(GrammarMode grammar) {
        if (_IsProgramRunning) {
          DAS.Error("Codelab.StartProgram.AlreadyRunning", "");
        }
        _IsProgramRunning = true;

        _StartDateTime = System.DateTime.UtcNow;
        _NumBlocksExecuted = 0;
        _WasProgramAborted = false;

        ResetProgramStateVars();

        switch (grammar) {
        case GrammarMode.None:
          DAS.Error("Codelab.StartProgram.NoneGrammar", "");
          break;
        case GrammarMode.Horizontal:
          DAS_Event("robot.code_lab.start_program_horizontal", "");
          break;
        case GrammarMode.Vertical:
          DAS_Event("robot.code_lab.start_program_vertical", "");

          break;
        }
      }

      public void EndProgram(GrammarMode grammar) {
        if (!_IsProgramRunning) {
          DAS.Error("Codelab.EndProgram.WasNotRunning", "");
        }
        _IsProgramRunning = false;

        double timeInProgram_s = (System.DateTime.UtcNow - _StartDateTime).TotalSeconds;

        switch (grammar) {
        case GrammarMode.None:
          DAS.Error("Codelab.EndProgram.NoneGrammar", "");
          break;
        case GrammarMode.Horizontal:
          if (_WasProgramAborted) {
            DAS_Event("robot.code_lab.aborted_program_horizontal", _NumBlocksExecuted.ToString(), DASUtil.FormatExtraData(timeInProgram_s.ToString()));
          }
          DAS_Event("robot.code_lab.end_program_horizontal", _NumBlocksExecuted.ToString(), DASUtil.FormatExtraData(timeInProgram_s.ToString()));
          break;
        case GrammarMode.Vertical:
          if (_WasProgramAborted) {
            DAS_Event("robot.code_lab.aborted_program_vertical", _NumBlocksExecuted.ToString(), DASUtil.FormatExtraData(timeInProgram_s.ToString()));
          }
          DAS_Event("robot.code_lab.end_program_vertical", _NumBlocksExecuted.ToString(), DASUtil.FormatExtraData(timeInProgram_s.ToString()));
          break;
        }
      }

      public void OnStopAll() {
        if (_IsProgramRunning) {
          _WasProgramAborted = true;
        }
      }
    } // End class ProgramState

    // SessionState:

    private ProjectStats _ProjectStats = new ProjectStats();
    private ProgramState _ProgramState = new ProgramState();
    private ChallengesState _ChallengesState = new ChallengesState();
    private TutorialState _TutorialState = new TutorialState();

    private System.DateTime _EnterCodeLabDateTime;
    private System.DateTime _EnterCurrentGrammarModeDateTime;
    private GrammarMode _CurrentGrammar = GrammarMode.None;
    private int _NumProgramsRun = 0;
    private int _NumProgramsRunInMode = 0;
    private bool _ReceivedGreenFlagEvent = false;

    public ProgramState GetProgramState() {
      return _ProgramState;
    }

    public bool IsProgramRunning() {
      return _ProgramState.IsProgramRunning();
    }

    public bool ShouldWaitForActions() {
      if (_CurrentGrammar == GrammarMode.Vertical) {
        return _ProgramState.ShouldWaitForActions();
      }
      return true;
    }

    public void SetShouldWaitForActions(bool newVal) {
      if (_CurrentGrammar == GrammarMode.Vertical) {
        _ProgramState.SetShouldWaitForActions(newVal);
      }
      else {
        DAS.Error("CodeLab.SetShouldWaitForActions", "Attempt to set in invalid grammar " + _CurrentGrammar.ToString());
      }
    }

    public bool IsAnimTrackEnabled(AnimTrack animTrack) {
      if (_CurrentGrammar == GrammarMode.Vertical) {
        return _ProgramState.IsAnimTrackEnabled(animTrack);
      }
      return true;
    }

    public void SetIsAnimTrackEnabled(AnimTrack animTrack, bool newVal) {
      if (_CurrentGrammar == GrammarMode.Vertical) {
        _ProgramState.SetIsAnimTrackEnabled(animTrack, newVal);
      }
      else {
        DAS.Error("CodeLab.SetIsAnimTrackEnabled", "Attempt to set in invalid grammar " + _CurrentGrammar.ToString());
      }
    }

    // Wrap DAS event so we can easily log the events for debugging
    public static void DAS_Event(string eventName, string eventValue, Dictionary<string, string> extraData = null) {
      DAS.Event(eventName, eventValue, extraData);
#if false // Enable for easier debugging the DAS events sent from CodeLab
      if (extraData != null) {
        DAS.Info(eventName, "s_val='" + eventValue + "' $data='" + extraData["$data"] + "'");
      }
      else {
        DAS.Info(eventName, "s_val='" + eventValue + "'");
      }
#endif
    }

    public void StartProgram() {
      _ProjectStats.PostPendingChanges(CodeLab.ProjectStats.EventCategory.updated_user_project);
      _ProgramState.StartProgram(_CurrentGrammar);
      ++_NumProgramsRun;
      ++_NumProgramsRunInMode;
    }

    public void EndProgram() {
      _ProgramState.EndProgram(_CurrentGrammar);
    }

    public void OnGreenFlagClicked() {
      DAS_Event("robot.code_lab.green_flag_clicked", "");
      _ReceivedGreenFlagEvent = true;
    }

    public void OnStopAll() {
      if (_ReceivedGreenFlagEvent) {
        // This was sent from the greenflag, and isn't a normal stop request
        _ReceivedGreenFlagEvent = false;
      }
      else {
        DAS_Event("robot.code_lab.stop_all", "");
      }

      _ProgramState.OnStopAll();
    }

    private void Reset() {
      _ProgramState.Reset();
      if (_CurrentGrammar != GrammarMode.None) {
        DAS.Error("Codelab.ResetActiveGrammar", "Should exit mode before reset!");
      }
      _NumProgramsRun = 0;
      _NumProgramsRunInMode = 0;
      _ReceivedGreenFlagEvent = false;
    }

    public void StartSession(GrammarMode grammarMode) {
      if (_CurrentGrammar != GrammarMode.None) {
        EndSession();
      }
      DAS_Event("robot.code_lab.open", "");
      Reset();
      _EnterCodeLabDateTime = System.DateTime.UtcNow;
      SetGrammarMode(grammarMode);
    }

    public void EndSession() {
      SetGrammarMode(GrammarMode.None);
      double timeInCodeLab_s = (System.DateTime.UtcNow - _EnterCodeLabDateTime).TotalSeconds;
      DAS_Event("robot.code_lab.close", _NumProgramsRun.ToString(), DASUtil.FormatExtraData(timeInCodeLab_s.ToString()));
    }

    private void ExitGrammarMode() {
      double timeInCodeLabMode_s = (System.DateTime.UtcNow - _EnterCurrentGrammarModeDateTime).TotalSeconds;

      switch (_CurrentGrammar) {
      case GrammarMode.None:
        DAS.Error("Codelab.ExitNoneGrammar", "");
        break;
      case GrammarMode.Horizontal:
        DAS_Event("robot.code_lab.end_horizontal", _NumProgramsRunInMode.ToString(), DASUtil.FormatExtraData(timeInCodeLabMode_s.ToString()));
        break;
      case GrammarMode.Vertical:
        DAS_Event("robot.code_lab.end_vertical", _NumProgramsRunInMode.ToString(), DASUtil.FormatExtraData(timeInCodeLabMode_s.ToString()));
        break;
      }

      _CurrentGrammar = GrammarMode.None;
    }

    private void EnterGrammarMode(GrammarMode newGrammar) {
      if (_CurrentGrammar != GrammarMode.None) {
        DAS.Error("Codelab.EnterAlreadyRunningGrammar", "CurrentGrammar = " + _CurrentGrammar);
      }

      switch (newGrammar) {
      case GrammarMode.None:
        DAS.Error("Codelab.EnterNoneGrammar", "");
        break;
      case GrammarMode.Horizontal:
        DAS_Event("robot.code_lab.start_horizontal", "");
        break;
      case GrammarMode.Vertical:
        DAS_Event("robot.code_lab.start_vertical", "");
        break;
      }

      _CurrentGrammar = newGrammar;
      _NumProgramsRunInMode = 0;
      _EnterCurrentGrammarModeDateTime = System.DateTime.UtcNow;
    }

    public void SetGrammarMode(GrammarMode newGrammar) {
      if (_CurrentGrammar != newGrammar) {
        if (_CurrentGrammar != GrammarMode.None) {
          ExitGrammarMode();
        }
        if (newGrammar != GrammarMode.None) {
          EnterGrammarMode(newGrammar);
        }
      }
    }

    public GrammarMode GetGrammarMode() {
      return _CurrentGrammar;
    }

    public void ScratchBlockEvent(string blockEventName, Dictionary<string, string> extraData = null) {
      DAS_Event("robot.code_lab.scratch_block", blockEventName, extraData);
      _ProgramState.OnBlockStarted();
    }

    public void OnUpdatedProject(CodeLabProject project) {
      _ProjectStats.Update(project);
    }

    public void OnCreatedProject(CodeLabProject project) {
      PlayerProfile defaultProfile = DataPersistenceManager.Instance.Data.DefaultProfile;
      DAS_Event("robot.code_lab.created_project", defaultProfile.CodeLabProjects.Count.ToString());
    }

    public void OnChallengesOpen() {
      _ChallengesState.Open();
    }

    public void OnChallengesClose() {
      _ChallengesState.Close();
    }

    public void OnChallengesSetSlideNumber(uint slideNum) {
      _ChallengesState.SetSlideNumber(slideNum);
    }

    public void OnTutorialOpen() {
      _TutorialState.Open();
    }

    public void OnTutorialClose() {
      _TutorialState.Close();
    }
  }
}