namespace WellFired.Shared.StateMachine
{
	/// <summary>
	/// The Updateable interface for a state. If your state implements this interface, it will have the Update method called.
	/// </summary>
	public interface IUpdateable
	{
		void Update();
	}
}