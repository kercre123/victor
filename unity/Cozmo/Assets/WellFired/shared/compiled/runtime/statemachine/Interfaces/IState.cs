namespace WellFired.Shared.StateMachine
{
	/// <summary>
	/// The Interface for a state, each state must at least implement the IState interface.
	/// States should be self contained pieces of logic. Each state will determine if it
	/// can move onto a new state and what new state it should move onto.
	/// </summary>
	public interface IState 
	{
		/// <summary>
		/// This method will be called when a state is entered, you can use this to set up any state dependant data.
		/// </summary>
		void OnEnterState();

		/// <summary>
		/// This method will be called when a state is exitted, you can use this to finalize the running state.
		/// </summary>
		void OnExitState();

		/// <summary>
		/// Determines whether this state can transition.
		/// </summary>
		/// <returns><c>true</c> if this state can transition; otherwise, <c>false</c>.</returns>
		bool CanTransition();

		/// <summary>
		/// Gets the next state
		/// </summary>
		/// <returns>The next state.</returns>
		IState GetNextState();
	}
}