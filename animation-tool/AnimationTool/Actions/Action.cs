namespace AnimationTool
{
    public interface Action
    {
        bool Do();
        void Undo();
    }
}
