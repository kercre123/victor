using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using G2U = Anki.Cozmo.ExternalInterface;
using U2G = Anki.Cozmo.U2G;

/// <summary>
/// The type of protocol to communicate over.
/// </summary>
public enum ChannelProtocol {
	Tcp = 0,
	Udp,
}

/// <summary>
/// The reason a connection has failed.
/// </summary>
public enum DisconnectionReason {
	FailedToListen,
	FailedToAdvertise,
	ConnectionLost,
	AttemptedToSendInvalidData,
	ReceivedInvalidData,
	ConnectionThrottled,

	// temp
	None,
	RobotDisconnected,
	RobotConnectionTimedOut,
	UnityReloaded,
}

/// <summary>
/// Base class for channels between C++ and C#.
/// </summary>
public abstract class ChannelBase {

	/// <summary>
	/// True if the connection is actively advertising or is established. Only valid from the main Unity thread.
	/// </summary>
	public bool IsActive { get; protected set; }

	/// <summary>
	/// True if the connection is established. Only valid from the main Unity thread.
	/// </summary>
	public bool IsConnected { get; protected set; }

	/// <summary>
	/// Occurs when the client is connected and ready to send to.
	/// </summary>
	public event Action<string> ConnectedToClient;

	/// <summary>
	/// Occurs when the client fails to connect or is disconnected. May happen synchronously in a Connect call.
	/// </summary>
	public event Action<DisconnectionReason> DisconnectedFromClient;

	/// <summary>
	/// Occurs when a valid message is received from the client.
	/// </summary>
	public event Action<G2U.MessageEngineToGame> MessageReceived;

	/// <summary>
	/// True when there is any network traffic (eg disconnection messages, asynchronous sends cleaning up).
	/// </summary>
	public abstract bool HasPendingOperations { get; }

	protected void RaiseConnectedToClient(string connectionInformation)
	{
		if (ConnectedToClient != null) {
			ConnectedToClient(connectionInformation);
		}
	}
	
	protected void RaiseDisconnectedFromClient(DisconnectionReason reason)
	{
		if (DisconnectedFromClient != null) {
			DisconnectedFromClient(reason);
		}
	}

	protected void RaiseMessageReceived(G2U.MessageEngineToGame message)
	{
		if (MessageReceived != null) {
			MessageReceived(message);
		}
	}

	/// <summary>
	/// Initializes the server, listening on the specified port and advertising its presence to the client.
	/// </summary>
	/// <param name="deviceID">A number representing this device.</param> 
	/// <param name="localPort">Local port to listen on.</param>
	/// <param name="advertisingIP">The ip to advertise to.</param>
	/// <param name="advertisingPort">The port to advertise to.</param>
	/// <remarks>Only call this from the main Unity thread.</remarks>
	public abstract void Connect (int deviceID, int localPort, string advertisingIP, int advertisingPort);

	/// <summary>
	/// Disconnects this server immediately. No DisconnectedFromClient event will be raised.
	/// </summary>
	/// <remarks>Only call this from the main Unity thread.</remarks>
	public abstract void Disconnect();

	/// <summary>
	/// Sends a message to the connected client.
	/// </summary>
	/// <param name="message">The message to send, which is serialized immediately.</param>
	/// <remarks>Only call this from the main Unity thread.</remarks>
	public abstract void Send(U2G.Message message);

	/// <summary>
	/// Updates the connection, allowing it to send events on the main Unity thread.
	/// </summary>
	/// <remarks>Only call this from the main Unity thread.</remarks>
	public abstract void Update();
}
