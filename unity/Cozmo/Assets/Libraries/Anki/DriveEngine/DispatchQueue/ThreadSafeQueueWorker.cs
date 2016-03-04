using System;

// source: https://raw.githubusercontent.com/neuecc/UniRx/master/Assets/UniRx/Scripts/InternalUtil/ThreadSafeQueueWorker.cs
// commit: e8feade0c6c9880f7c3a6df7c8c61d2e2efde74c

namespace Anki.DispatchQueue {
  public class ThreadSafeQueueWorker {
    const int InitialSize = 32;
		
    object gate = new object();
    bool dequing = false;
		
    int actionListCount = 0;
    Action[] actionList = new Action[InitialSize];
		
    int waitingListCount = 0;
    Action[] waitingList = new Action[InitialSize];
    		
    public void Enqueue(Action action) {
      lock (gate) {
        if (dequing) {
          if (waitingList.Length == waitingListCount) {
            var newArray = new Action[checked(waitingListCount * 2)];
            Array.Copy(waitingList, newArray, waitingListCount);
            waitingList = newArray;
          }
          waitingList[waitingListCount++] = action;
        }
        else {
          if (actionList.Length == actionListCount) {
            var newArray = new Action[checked(actionListCount * 2)];
            Array.Copy(actionList, newArray, actionListCount);
            actionList = newArray;
          }
          actionList[actionListCount++] = action;
        }
      }
    }
		
    public void ExecuteAll(Action<Exception> unhandledExceptionCallback) {
      lock (gate) {
        if (actionListCount == 0)
          return;
				
        dequing = true;
      }
			
      for (int i = 0; i < actionListCount; i++) {
        var action = actionList[i];
				
        try {
          action();
        }
        catch (Exception ex) {
          unhandledExceptionCallback(ex);
        }
      }
			
      lock (gate) {
        dequing = false;
        Array.Clear(actionList, 0, actionListCount);
				
        var swapTempActionList = actionList;
				
        actionListCount = waitingListCount;
        actionList = waitingList;
				
        waitingListCount = 0;
        waitingList = swapTempActionList;
      }
    }
  }
}