using System.Collections.Generic;

namespace AnimationTool
{
    public class ActionManager
    {
        private static ActionManager instance;
        private static List<Action> actions { get {  return instance == null ? null : instance._actions; } }
        private static Action currentAction { get { return instance == null ? null : instance._currentAction; } }
        private static int currentActionIndex { get { return instance == null ? -1 : instance._currentActionIndex; } set { if (instance != null) instance._currentActionIndex = value; } }

        private List<Action> _actions;
        private int _currentActionIndex;
        private Action _currentAction { get { return actions == null || currentActionIndex < 0 || currentActionIndex >= actions.Count ? null : actions[currentActionIndex]; } }

        static ActionManager()
        {
            instance = new ActionManager();
        }

        public ActionManager()
        {
            _actions = new List<Action>();
            _currentActionIndex = -1;
        }

        public static void Reset()
        {
            if (actions == null) return;

            actions.Clear();
        }

        public static void Undo()
        {
            if (currentAction == null) return;

            currentAction.Undo();
            --currentActionIndex;
        }

        public static void Redo()
        {
            ++currentActionIndex;

            if (currentAction == null) return;

            currentAction.Do();
        }

        public static bool Do(Action action, bool canNotBeUndone = false)
        {
            if (actions == null || !action.Do()) return false;

            if (canNotBeUndone) return true;

            if (++currentActionIndex < actions.Count) // if not at the end of action list, remove all undid actions
            {
                actions.RemoveRange(currentActionIndex, actions.Count - currentActionIndex);
            }

            actions.Add(action);

            return true;
        }
    }
}
