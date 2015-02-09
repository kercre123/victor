using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;

public class UdpChannel : ChannelBase {

	private class SocketBufferState
	{
		public Socket socket;
		public byte[] buffer;
	}

	private struct QueuedSend
	{
		public SocketBufferState state;
		public int length;

		public QueuedSend(SocketBufferState state, int size)
		{
			this.state = state;
			this.length = size;
		}
	}

	private enum ConnectionState {
		Disconnected,
		Advertising,
		WaitingForConnection,
		Connected,
		ConnectedAndSending,
	}

	// various constants
	private const int SendBufferSize = 8192;
	private const int ReceiveBufferSize = 8192;
	private const int SendBufferPoolLength = 66;
	private const int ReceiveBufferPoolLength = 2;
	private const int BufferStatePoolLength = 68;
	private const int MaxQueuedSends = 64;
	private const int MaxQueuedReceives = 64;
	private const int MaxQueuedLogs = 64;

	// unique object used for locking
	private readonly object sync = new object();

	// sockets
	private readonly AdvertisementRegistrationMsg advertisementRegistrationMessage = new AdvertisementRegistrationMsg ();
	private Socket mainServer;
	private Socket advertisingClient;
	private readonly IPEndPoint anyEndPoint = new IPEndPoint(IPAddress.Any, 0);
	private IPEndPoint mainEndPoint = null;
	private IPEndPoint advertisementEndPoint = null;

	// state information
	private ConnectionState connectionState = ConnectionState.Disconnected;
	private string currentConnectionString = null;
	private DisconnectionReason currentDisconnectionReason = DisconnectionReason.ConnectionLost;

	// various queues
	private readonly List<object> queuedLogs = new List<object>(MaxQueuedLogs);
	private readonly Queue<QueuedSend> sentBuffers = new Queue<QueuedSend>(MaxQueuedSends);
	private readonly List<NetworkMessage> receivedMessages = new List<NetworkMessage> (MaxQueuedReceives);

	// lists of pooled objects
	private readonly List<byte[]> sendBufferPool = new List<byte[]>(SendBufferPoolLength);
	private readonly List<byte[]> receiveBufferPool = new List<byte[]>(ReceiveBufferPoolLength);
	private readonly List<SocketBufferState> bufferStatePool = new List<SocketBufferState>(BufferStatePoolLength);

	// asynchronous callbacks (saved to reduce allocations)
	private readonly AsyncCallback callback_ChangeAdvertisement_StartComplete;
	private readonly AsyncCallback callback_ChangeAdvertisement_StopComplete;
	private readonly AsyncCallback callback_ServerReceive_Complete;
	private readonly AsyncCallback callback_ServerSend_Complete;

	public UdpChannel()
	{
		callback_ChangeAdvertisement_StartComplete = ChangeAdvertisement_StartComplete;
		callback_ChangeAdvertisement_StopComplete = ChangeAdvertisement_StopComplete;
		callback_ServerReceive_Complete = ServerReceive_Complete;
		callback_ServerSend_Complete = ServerSend_Complete;
	}

	public override void Connect(int deviceID, int localPort, string advertisingIP, int advertisingPort)
	{
		IPAddress advertisingAddress;
		if (!IPAddress.TryParse (advertisingIP, out advertisingAddress)) {
			throw new ArgumentException("Could not parse ip address.", "advertisingIP");
		}

		// dunno if 0 is good or not, so allowing it
		if (deviceID < 0 || deviceID > byte.MaxValue) {
			throw new ArgumentException("Device id must be 0 to 255.", "deviceID");
		}
		
		if (IsActive) {
			throw new InvalidOperationException("UdpChannel is already connecting. Disconnect first.");
		}

		lock (sync) {
			// should only become active through this call
			if (connectionState != ConnectionState.Disconnected) {
				throw new InvalidOperationException("You should only call Connect on the main Unity thread.");
			}

			try {
				// set up main socket
				IPEndPoint localEndPoint = new IPEndPoint (IPAddress.Any, localPort);
				mainServer = new Socket(localEndPoint.AddressFamily, SocketType.Dgram, ProtocolType.Udp);
				mainServer.Bind (localEndPoint);

				ServerReceive();
			}
			catch (Exception e) {
				CleanSynchronously(DisconnectionReason.FailedToListen, e);
				return;
			}

			try {
				// set up advertisement message
				string localIP = ((IPEndPoint)mainServer.LocalEndPoint).Address.ToString();
				int length = Encoding.UTF8.GetByteCount(localIP);
				if (length + 1 > advertisementRegistrationMessage.ip.Length) {
					CleanSynchronously(DisconnectionReason.FailedToAdvertise, "Advertising host is too long.");
					return;
				}
				Encoding.UTF8.GetBytes (localIP, 0, localIP.Length, advertisementRegistrationMessage.ip, 0);
				advertisementRegistrationMessage.ip[length] = 0;
				
				advertisementRegistrationMessage.port = (ushort)localPort;
				advertisementRegistrationMessage.id = (byte)deviceID;
				advertisementRegistrationMessage.protocol = (byte)ChannelProtocol.Udp;
				advertisementRegistrationMessage.enableAdvertisement = 1;

				// set up advertisement socket
				advertisementEndPoint = new IPEndPoint(advertisingAddress, advertisingPort);
				advertisingClient = new Socket(advertisementEndPoint.AddressFamily, SocketType.Dgram, ProtocolType.Udp);

				// advertise
				ChangeAdvertisement(true);
			}
			catch (Exception e) {
				CleanSynchronously(DisconnectionReason.FailedToAdvertise, e);
				return;
			}

			// set state
			connectionState = ConnectionState.Advertising;
			IsActive = true;
			IsConnected = false;

			Debug.Log ("UdpConnection: Listening on " + mainServer.LocalEndPoint.ToString () + ".");
		}
	}

	public override void Disconnect ()
	{
		// IsActive only becomes true synchronously
		if (IsActive) {

			lock (sync) {
				if (connectionState != ConnectionState.Disconnected) {
					Clean (DisconnectionReason.ConnectionLost);
					IsActive = false;
					IsConnected = false;
					ProcessLogs();
					Debug.Log ("UdpConnection: Disconnected manually.");
				}
			}
		}
	}

	public override void Send(NetworkMessage message)
	{
		if (message.ID < 0 || message.ID > 255) {
			throw new ArgumentException("Message id is not in valid range.", "message");
		}

		// IsConnected only becomes true synchronously
		if (!IsConnected) {
			throw new InvalidOperationException("Can only send when connected.");
		}

		lock (sync) {
			// about to be disconnected; ignore
			if (connectionState != ConnectionState.Connected && connectionState != ConnectionState.ConnectedAndSending) {
				Debug.LogWarning("Ignoring message of type " + message.GetType().FullName + " because the connection is about to disconnect.");
				return;
			}

			bool success = false;
			SocketBufferState state = AllocateSendBuffer(mainServer);
			try {
				int index = 0;
				try {
					byte id = (byte)message.ID;
					SerializerUtility.Serialize(state.buffer, ref index, id);
					message.Serialize (state.buffer, ref index);
				} catch (IndexOutOfRangeException) {
					CleanSynchronously(DisconnectionReason.AttemptedToSendInvalidData,
					                   "Send buffer is not large enough to serialize " + message.GetType ().FullName + ".");
					return;
				}
				catch (Exception e)
				{
					CleanSynchronously(DisconnectionReason.AttemptedToSendInvalidData, e);
					return;
				}

				if (connectionState == ConnectionState.Connected) {
					try {
						ServerSend (state, index);
						success = true;
					}
					catch (Exception e) {
						CleanSynchronously(DisconnectionReason.ConnectionLost, e);
						return;
					}
				}
				else {
					if (sentBuffers.Count < MaxQueuedSends) {
						sentBuffers.Enqueue (new QueuedSend(state, index));
						success = true;
					}
					else {
						CleanSynchronously(DisconnectionReason.ConnectionThrottled,
						                   "Disconnecting. Too much data queued to send.");
						return;
					}
				}
			}
			finally {
				if (!success) {
					FreeSendBuffer(state);
				}
			}
		}
	}

	public override void Update()
	{
		// IsActive only becomes true synchronously
		if (IsActive) {

			lock (sync) {
				InternalUpdate ();
			}
		}
	}
	
	private void InternalUpdate()
	{
		ProcessLogs();
		
		if (!IsConnected && (connectionState == ConnectionState.Connected || connectionState == ConnectionState.ConnectedAndSending)) {
			IsConnected = true;
			Debug.Log ("UdpConnection: Connected to " + currentConnectionString + ".");
			try {
				RaiseConnectedToClient(currentConnectionString);
			}
			catch (Exception e) {
				Debug.LogException(e);
			}
			
		}
		
		ProcessMessages();
		
		IsConnected = (connectionState == ConnectionState.Connected || connectionState == ConnectionState.ConnectedAndSending);
		IsActive = (connectionState != ConnectionState.Disconnected);
		if (!IsActive) {
			try {
				RaiseDisconnectedFromClient(currentDisconnectionReason);
			}
			catch (Exception e) {
				Debug.LogException(e);
			}
		}
	}
	
	private void ProcessLogs()
	{
		int logCount = queuedLogs.Count;
		if (logCount != 0) {
			for (int i = 0; i < logCount; ++i) {
				object log = queuedLogs [i];
				if (log is Exception) {
					Debug.LogException ((Exception)log);
				} else {
					Debug.LogError ("UdpConnection: " + log);
				}
			}
			queuedLogs.Clear ();
		}
	}
	
	private void ProcessMessages()
	{
		int count = receivedMessages.Count;
		if (count != 0) {
			for (int i = 0; i < count; ++i) {
				try {
					RaiseMessageReceived(receivedMessages[i]);
				}
				catch (Exception e) {
					Debug.LogException(e);
				}
			}
			receivedMessages.Clear ();
		}
	}
	
	private void Clean(DisconnectionReason reason) {
		
		bool needStopAdvertise = (connectionState == ConnectionState.WaitingForConnection ||
		                          connectionState == ConnectionState.Connected ||
		                          connectionState == ConnectionState.ConnectedAndSending);
		
		connectionState = ConnectionState.Disconnected;
		currentConnectionString = null;
		currentDisconnectionReason = reason;
		sentBuffers.Clear ();
		receivedMessages.Clear ();

		if (advertisingClient != null) {
			if (needStopAdvertise) {
				ChangeAdvertisement(false);
			}
			else {
				advertisingClient.Close();
			}
			advertisingClient = null;
		}
		
		if (mainServer != null) {
			mainServer.Close();
			mainServer = null;
		}
		
		mainEndPoint = null;
		advertisementEndPoint = null;
	}

	private void Clean(DisconnectionReason reason, object exceptionOrLog)
	{
		Clean (reason);
	    queuedLogs.Add (exceptionOrLog);
	}

	private void CleanSynchronously(DisconnectionReason reason, object exceptionOrLog)
	{
		Clean (reason, exceptionOrLog);
		InternalUpdate ();
	}
	
	private void ServerSend(SocketBufferState state, int length)
	{
		// should indicate programmer error
		if (connectionState != ConnectionState.Connected) {
			throw new InvalidOperationException("Error attempting to send on UdpConnection with state " + connectionState.ToString() + ".");
		}

		connectionState = ConnectionState.ConnectedAndSending;

		AsyncCallback callback = callback_ServerSend_Complete;
		BeginSendUdp(mainServer, state.buffer, length, mainEndPoint, callback, state);
	}

	private void ServerSend_Complete(IAsyncResult result)
	{
		lock (sync) {
			SocketBufferState state = (SocketBufferState)result.AsyncState;
			Socket socket = state.socket;
			FreeSendBuffer(state);

			// this is an old connection
			if (socket != mainServer) {
				return;
			}

			try {
				socket.EndSendTo(result);
				
				connectionState = ConnectionState.Connected;

				if (sentBuffers.Count > 0) {
					QueuedSend queuedSend = sentBuffers.Dequeue();
					bool success = false;
					try {
						ServerSend (queuedSend.state, queuedSend.length);
						success = true;
					}
					finally {
						if (!success) {
							FreeSendBuffer (queuedSend.state);
						}
					}
				}
			}
			catch (Exception e) {
				Clean (DisconnectionReason.ConnectionLost, e);
			}
		}
	}

	private void ServerReceive()
	{
		SocketBufferState state = AllocateReceiveBuffer(mainServer);
		bool success = false;
		try {
			EndPoint endPoint = anyEndPoint;
			mainServer.BeginReceiveFrom(state.buffer, 0, state.buffer.Length, SocketFlags.None, ref endPoint, callback_ServerReceive_Complete, state);
			success = true;
		}
		finally {
			if (!success) {
				FreeReceiveBuffer(state);
			}
		}
	}

	private void ServerReceive_Complete(IAsyncResult result) 
	{
		lock (sync) {
			SocketBufferState state = (SocketBufferState)result.AsyncState;
			try {
				// this is an old connection
				if (state.socket != mainServer) {
					return;
				}

				try {
					EndPoint endPoint = anyEndPoint;
					int length = state.socket.EndReceiveFrom(result, ref endPoint);

					IPEndPoint ipEndPoint = endPoint as IPEndPoint;
					if (ipEndPoint == null) {

						if (connectionState == ConnectionState.Advertising || connectionState == ConnectionState.WaitingForConnection) {
							connectionState = ConnectionState.Connected;
							mainEndPoint = ipEndPoint;
						}
						/*else ? */
						if (mainEndPoint.Equals (ipEndPoint)) {
							if (!ParseMessage (state.buffer, length)) {
								return;
							}
						}
					}
					
					ServerReceive();
				}
				catch (Exception e) {
					Clean(DisconnectionReason.ConnectionLost, e);
				}
			}
			finally {
				FreeReceiveBuffer(state);
			}
		}
	}

	private bool ParseMessage(byte[] buffer, int length)
	{
		int index = 0;
		byte id;
		try {
			SerializerUtility.Deserialize (buffer, ref index, out id);
		}
		catch (IndexOutOfRangeException) {
			Clean (DisconnectionReason.ReceivedInvalidData,
			       "Disconnecting. Message too short to even specify id.");
			return false;
		}

		NetworkMessage message = null;
		try {
			message = NetworkMessageCreation.Allocate((int)id);
			if (message == null) {
				Clean (DisconnectionReason.ReceivedInvalidData,
				       "Disconnecting. Unknown message type received with id " + id.ToString() + ".");
				return false;
			}
			
			message.Deserialize(buffer, ref index);
		}
		catch (IndexOutOfRangeException) {
			Clean (DisconnectionReason.ReceivedInvalidData,
			       "Disconnecting. Message not long enough for message of type " +
			       (message == null ? id.ToString() : message.GetType().FullName) + ".");
			return false;
		}
		catch (Exception e) {
			Clean(DisconnectionReason.ReceivedInvalidData, e);
			return false;
		}

		if (index != length) {
			Clean (DisconnectionReason.ReceivedInvalidData,
			       "Disconnecting. Message too long in receive buffer at end of message of type " + message.GetType ().FullName + ".");
			return false;
		}

		if (receivedMessages.Count >= MaxQueuedReceives) {
			Clean (DisconnectionReason.ConnectionThrottled,
			       "Too many messages received too quickly.");
			return false;
		}

		receivedMessages.Add (message);
		return true;
	}
	
	private void ChangeAdvertisement(bool start)
	{
		advertisementRegistrationMessage.enableAdvertisement = start ? (byte)1 : (byte)0;

		bool success = false;
		SocketBufferState state = AllocateSendBuffer(advertisingClient);
		try {
			int index = 0;
			try {
				advertisementRegistrationMessage.Serialize(state.buffer, ref index);
			}
			catch (IndexOutOfRangeException e) {
				throw new IndexOutOfRangeException("Send buffer is not large enough for advertisement registration message.", e);
			}

			AsyncCallback callback = start ? callback_ChangeAdvertisement_StartComplete : callback_ChangeAdvertisement_StopComplete;
			BeginSendUdp(advertisingClient, state.buffer, index, advertisementEndPoint, callback, state);

			success = true;
		}
		finally {
			if (!success) {
				FreeSendBuffer(state);
			}
		}
	}

	private void ChangeAdvertisement_StartComplete(IAsyncResult result)
	{
		lock (sync) {
			SocketBufferState state = (SocketBufferState)result.AsyncState;
			Socket socket = state.socket;
			FreeSendBuffer(state);

			// this is an old connection
			if (socket != advertisingClient) {
				return;
			}
			
			try {
				socket.EndSendTo(result);

				if (connectionState == ConnectionState.Advertising) {
					connectionState = ConnectionState.WaitingForConnection;
				}
			}
			catch (Exception e) {
				Clean(DisconnectionReason.FailedToAdvertise, e);
			}
		}
	}

	private void ChangeAdvertisement_StopComplete(IAsyncResult result)
	{
		lock (sync) {
			SocketBufferState state = (SocketBufferState)result.AsyncState;
			Socket socket = state.socket;
			FreeSendBuffer(state);

			try {
				// always an old connection

				socket.EndSendTo(result);
			}
			catch (Exception) {
				// ignore, since we can't do anything
			}
			// old connection, so need to close
			finally {
				state.socket.Close ();
			}
		}
	}

	private SocketBufferState AllocateSendBuffer(Socket socket)
	{
		byte[] buffer;
		int count = sendBufferPool.Count;
		if (count != 0) {
			buffer = sendBufferPool [count - 1];
			sendBufferPool.RemoveAt (count - 1);
		} else {
			buffer = new byte[SendBufferSize];
		}
		return AllocateBufferState(socket, buffer);
	}
	
	private void FreeSendBuffer(SocketBufferState bufferState)
	{
		if (sendBufferPool.Count < SendBufferPoolLength) {
			sendBufferPool.Add (bufferState.buffer);
		}
		FreeBufferState (bufferState);
	}

	private SocketBufferState AllocateReceiveBuffer(Socket socket)
	{
		byte[] buffer;
		int count = receiveBufferPool.Count;
		if (count != 0) {
			buffer = receiveBufferPool[count - 1];
			receiveBufferPool.RemoveAt (count - 1);
		}
		else {
		    buffer = new byte[ReceiveBufferSize];
		}
		return AllocateBufferState (socket, buffer);
	}
	
	private void FreeReceiveBuffer(SocketBufferState bufferState)
	{
		if (receiveBufferPool.Count < ReceiveBufferPoolLength) {
			receiveBufferPool.Add (bufferState.buffer);
		}
		FreeBufferState (bufferState);
	}

    private SocketBufferState AllocateBufferState(Socket socket, byte[] buffer)
	{
		SocketBufferState state;
		int count = bufferStatePool.Count;
		if (count == 0) {
			state = bufferStatePool [count - 1];
			bufferStatePool.RemoveAt (count - 1);
		} else {
			state = new SocketBufferState ();
		}
		state.socket = socket;
		state.buffer = buffer;
		return state;
	}

	private void FreeBufferState(SocketBufferState state)
	{
		if (bufferStatePool.Count < BufferStatePoolLength) {
			bufferStatePool.Add (state);
		}
	}

	private static void BeginSendUdp(Socket socket, byte[] datagram, int bytes, IPEndPoint endPoint, AsyncCallback callback, object state)
	{
		try
		{
			socket.BeginSendTo (datagram, 0, bytes, SocketFlags.None, endPoint, callback, state);
		}
		catch (SocketException ex)
		{
			if (ex.ErrorCode == 10013)
			{
				socket.SetSocketOption (SocketOptionLevel.Socket, SocketOptionName.Broadcast, 1);
				socket.BeginSendTo (datagram, 0, bytes, SocketFlags.None, endPoint, callback, state);
			}
			else {
				throw;
			}
		}
	}
}
