namespace WellFired.Shared.StateMachine
{
	/// <summary>
	/// The state machine class, you can instantiate one of these everywhere you want a state machine.
	/// 
	/// States should be self contained pieces of logic. Each state will determine if it
	/// can move onto a new state and what new state it should move onto, that's why the Switch State 
	/// method is not exposed.
	/// 
	/// You can query the Current State
	/// </summary>
	public sealed class StateMachine
	{
		/// <summary>
		/// Gets the current State.
		/// For state checking, you can do something like this : 
		/// if(stateMachine.Current is MyState)
		/// 	DoSomeLogic();
		/// else
		/// 	DoSomeOtherLogic();
		/// </summary>
		/// <value>The state of the current.</value>
		public IState CurrentState
		{
			get;
			private set;
		}

		/// <summary>
		/// Initializes a new instance of the <see cref="WellFired.GameMaker.StateMachine"/> class with a specific state
		/// </summary>
		/// <param name="initialState">Initial state.</param>
		public StateMachine(IState initialState)
		{
			SwitchState(initialState);
		}

		/// <summary>
		/// You must call the update method on any state machine constructed, this will drive the state machine.
		/// </summary>
		public void Update()
		{
			if(CurrentState == default(IState))
				return;

			if(CurrentState is IUpdateable)
				(CurrentState as IUpdateable).Update();

			var canTransitionToNextState = CurrentState.CanTransition();

			if(canTransitionToNextState)
				SwitchState(CurrentState.GetNextState());
		}

		/// <summary>
		/// Switchs the state.
		/// </summary>
		/// <param name="nextState">Next state.</param>
		private void SwitchState(IState nextState)
		{
			UnityEngine.Debug.Log(string.Format("Leaving State : {0}", CurrentState));

			if(CurrentState != default(IState))
				CurrentState.OnExitState();

			CurrentState = nextState;

			UnityEngine.Debug.Log(string.Format("Entering State : {0}", CurrentState));

			CurrentState.OnEnterState();
		}
	}
}