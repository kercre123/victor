using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Net.NetworkInformation;

public class UdpChannel : ChannelBase {

	private class SocketBufferState : IDisposable
	{
		public readonly UdpChannel channel;
		public Socket socket;
		public EndPoint endPoint;
		public readonly byte[] buffer;
		public int length;
		public SocketBufferState chainedSend;
		public bool isSocketActive;
		public bool needsCloseWhenDone;

		public SocketBufferState(UdpChannel channel, Socket socket, EndPoint endPoint)
		{
			this.channel = channel;
			this.buffer = new byte[MaxBufferSize];
			Reset (socket, endPoint);
		}

		public void Reset(Socket socket, EndPoint endPoint)
		{
			this.socket = socket;
			this.endPoint = endPoint;
			this.length = 0;
			this.chainedSend = null;
			this.isSocketActive = true;
			this.needsCloseWhenDone = false;
		}

		public void Dispose()
		{
			channel.FreeBufferState (this);
			this.socket = null;
			this.length = 0;
			this.chainedSend = null;
			this.isSocketActive = false;
			this.needsCloseWhenDone = false;
		}
	}

	private enum ConnectionState {
		Disconnected,
		Advertising,
		Connected,
	}

	// various constants
	private const int MaxBufferSize = 8192;
	private const int BufferStatePoolLength = 1280;
	private const int MaxQueuedSends = 640;
	private const int MaxQueuedReceives = 640;
	private const int MaxQueuedLogs = 640;

	// unique object used for locking
	private readonly object sync = new object();

	// fixed messages
	private readonly AdvertisementRegistrationMsg advertisementRegistrationMessage = new AdvertisementRegistrationMsg ();
	private readonly U2G_DisconnectFromUiDevice disconnectionMessage = new U2G_DisconnectFromUiDevice();

	// sockets
	private readonly IPEndPoint anyEndPoint = new IPEndPoint(IPAddress.Any, 0);
	private Socket advertisementClient;
	private IPEndPoint advertisementEndPoint = null;
	private Socket mainServer;
	private IPEndPoint mainEndPoint = null;

	// active operations
	private SocketBufferState advertisementSend = null;
	private SocketBufferState mainSend = null;
	private SocketBufferState mainReceive = null;

	// state information
	private ConnectionState connectionState = ConnectionState.Disconnected;
	private DisconnectionReason currentDisconnectionReason = DisconnectionReason.ConnectionLost;
	private int pendingOperations = 0;

	// timers
	private const float AdvertiseTick = .25f;
	private const float AdvertiseTimeout = 30.0f;
	private const float PingTimeout = 0.1f;
	private const float ReceiveTimeout = 1.0f;
	private float lastUpdateTime = 0;
	private float lastAdvertiseTime = 0;
	private float startAdvertiseTime = 0;
	private float lastReceiveTime = 0;
	private float lastPingTime = 0;

	// various queues
	private readonly List<object> queuedLogs = new List<object>(MaxQueuedLogs);
	private readonly Queue<SocketBufferState> sentBuffers = new Queue<SocketBufferState>(MaxQueuedSends);
	private readonly List<NetworkMessage> receivedMessages = new List<NetworkMessage> (MaxQueuedReceives);

	// lists of pooled objects
	private readonly List<SocketBufferState> bufferStatePool = new List<SocketBufferState>(BufferStatePoolLength);
	private readonly ByteSerializer serializer = new ByteSerializer (
#if CHANNEL_USE_NATIVE_ARCHITECTURE
		ByteSwappingArchitecture.Native
#else
		ByteSwappingArchitecture.LittleEndian
#endif
		);

	// asynchronous callbacks (saved to reduce allocations)
	private readonly AsyncCallback callback_SendAdvertisement_Complete;
	private readonly AsyncCallback callback_ServerReceive_Complete;
	private readonly AsyncCallback callback_ServerSend_Complete;
	private readonly AsyncCallback callback_SimpleSend_Complete;
    
	public override bool HasPendingOperations {
		get {
			lock (sync) {
				return (pendingOperations != 0);
			}
		}
	}

	public UdpChannel()
	{
		callback_SendAdvertisement_Complete = SendAdvertisement_Complete;
		callback_ServerReceive_Complete = ServerReceive_Complete;
		callback_ServerSend_Complete = ServerSend_Complete;
		callback_SimpleSend_Complete = SimpleSend_Complete;
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
			throw new InvalidOperationException("UdpChannel is already active. Disconnect first.");
		}

		lock (sync) {
			// should only become active through this call
			if (connectionState != ConnectionState.Disconnected) {
				throw new InvalidOperationException("You should only call Connect on the main Unity thread.");
			}

			lastUpdateTime = Time.realtimeSinceStartup;
			IPAddress localAddress = GetLocalIPv4();

			try {
				disconnectionMessage.deviceID = (byte)deviceID;

				// set up main socket
				IPEndPoint localEndPoint = anyEndPoint;//new IPEndPoint (/*localAddress ?? */ IPAddress.Any, localPort);
				mainServer = new Socket(localEndPoint.AddressFamily, SocketType.Dgram, ProtocolType.Udp);
				mainServer.Bind (localEndPoint);

				ServerReceive();
			}
			catch (Exception e) {
				DestroySynchronously(DisconnectionReason.FailedToListen, e);
				return;
			}

			try {
				string localIP = (localAddress ?? IPAddress.Loopback).ToString();

				// set up advertisement message
				Debug.Log ("Local IP: " + localIP);
				int length = Encoding.UTF8.GetByteCount(localIP);
				if (length + 1 > advertisementRegistrationMessage.ip.Length) {
					DestroySynchronously(DisconnectionReason.FailedToAdvertise,
					                   "Advertising host is too long: " +
					                   advertisementRegistrationMessage.ip.Length.ToString() + " bytes allowed, " +
					                   length.ToString () + " bytes used." );
					return;
				}
				Encoding.UTF8.GetBytes (localIP, 0, localIP.Length, advertisementRegistrationMessage.ip, 0);
				advertisementRegistrationMessage.ip[length] = 0;
				
				advertisementRegistrationMessage.port = (ushort)localPort;
				advertisementRegistrationMessage.id = (byte)deviceID;
				advertisementRegistrationMessage.protocol = (byte)ChannelProtocol.Udp;
				advertisementRegistrationMessage.enableAdvertisement = 1;
				advertisementRegistrationMessage.oneShot = 1;

				// set up advertisement socket
				advertisementEndPoint = new IPEndPoint(advertisingAddress, advertisingPort);
				advertisementClient = new Socket(advertisementEndPoint.AddressFamily, SocketType.Dgram, ProtocolType.Udp);

				// advertise
				startAdvertiseTime = lastUpdateTime;
				SendAdvertisement();
			}
			catch (Exception e) {
				DestroySynchronously(DisconnectionReason.FailedToAdvertise, e);
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
					Destroy (DisconnectionReason.ConnectionLost);
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
			if (connectionState != ConnectionState.Connected) {
				Debug.LogWarning("Ignoring message of type " + message.GetType().FullName + " because the connection is about to disconnect.");
				return;
			}

			bool success = false;
			SocketBufferState state = AllocateBufferState(mainServer, mainEndPoint);
			try {
				serializer.ByteBuffer = state.buffer;
				try {
					NetworkMessageSerializer.Serialize(serializer, message);
					state.length = serializer.Index;
				}
				catch (Exception e)
				{
					DestroySynchronously(DisconnectionReason.AttemptedToSendInvalidData, e);
					return;
				}
				finally {
					serializer.Clear ();
				}

				if (mainSend == null) {
					try {
						ServerSend (state);
					}
					catch (Exception e) {
						DestroySynchronously(DisconnectionReason.ConnectionLost, e);
						return;
					}
				}
				else {
					if (sentBuffers.Count < MaxQueuedSends) {
						sentBuffers.Enqueue (state);
					}
					else {
						DestroySynchronously(DisconnectionReason.ConnectionThrottled,
						                   "Disconnecting. Too many messages queued to send. (" + MaxQueuedSends.ToString() + " messages allowed.)");
						return;
					}
				}
				success = true;
			}
			finally {
				if (!success) {
					state.Dispose();
				}
			}
		}
	}

	public override void Update()
	{
		// IsActive only becomes true synchronously
		if (IsActive) {

			lock (sync) {
				lastUpdateTime = Time.realtimeSinceStartup;

				if (connectionState == ConnectionState.Advertising) {
					if (startAdvertiseTime + AdvertiseTimeout < lastUpdateTime) {
						DestroySynchronously(DisconnectionReason.ConnectionLost, "Connection attempt timed out after " + (lastUpdateTime - startAdvertiseTime).ToString("0.2f") + " seconds.");
						return;
					}

					try {
						SendAdvertisement();
					}
					catch (Exception e) {
						DestroySynchronously(DisconnectionReason.FailedToAdvertise, e);
						return;
					}
				}

				if (connectionState == ConnectionState.Connected) {
					if (lastReceiveTime + ReceiveTimeout < lastUpdateTime) {
						DestroySynchronously(DisconnectionReason.ConnectionLost, "Connection timed out after " + (lastUpdateTime - lastReceiveTime).ToString("0.2f") + " seconds.");
						return;
					}
				}

				InternalUpdate ();

				if (connectionState == ConnectionState.Connected) {
					if (lastPingTime + PingTimeout < lastUpdateTime) {
						//Ping();
					}
				}
			}
		}
	}
	
	private void InternalUpdate()
	{
		ProcessLogs();
		
		if (!IsConnected && (connectionState == ConnectionState.Connected)) {
			IsConnected = true;
			Debug.Log ("UdpConnection: Connected to " + mainEndPoint.ToString() + ".");
			try {
				RaiseConnectedToClient(mainEndPoint.ToString());
			}
			catch (Exception e) {
				Debug.LogException(e);
			}
			
		}
		
		ProcessMessages();

		bool wasActive = IsActive;
		IsConnected = (connectionState == ConnectionState.Connected);
		IsActive = (connectionState != ConnectionState.Disconnected);

		if (wasActive && !IsActive) {
			Debug.LogWarning("UdpConnection: Disconnected. Reason is " + currentDisconnectionReason.ToString() + "."); 

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
					Debug.LogWarning ("UdpConnection: " + log);
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
	
	private void Destroy(DisconnectionReason reason) {

		bool needsDisconnect = (connectionState == ConnectionState.Connected && mainServer != null && mainEndPoint != null);

		connectionState = ConnectionState.Disconnected;
		currentDisconnectionReason = reason;
		
		sentBuffers.Clear ();
		receivedMessages.Clear ();
		
		if (!needsDisconnect || !SendDisconnect ()) {
			if (mainServer != null) {
				mainServer.Close ();
			}
			if (mainSend != null) {
				mainSend.isSocketActive = false;
			}
			if (mainReceive != null) {
				mainReceive.isSocketActive = false;
			}
		}
		mainServer = null;
		mainSend = null;
		mainReceive = null;
		mainEndPoint = null;

		if (advertisementClient != null) {
			advertisementClient.Close();
			advertisementClient = null;
		}
		if (advertisementSend != null) {
			advertisementSend.isSocketActive = false;
			advertisementSend = null;
		}
		advertisementEndPoint = null;
	}

	private void Destroy(DisconnectionReason reason, object exceptionOrLog)
	{
		Destroy (reason);
	    queuedLogs.Add (exceptionOrLog);
	}

	private void DestroySynchronously(DisconnectionReason reason, object exceptionOrLog)
	{
		Destroy (reason, exceptionOrLog);
		InternalUpdate ();
	}
	
	private bool SendDisconnect()
	{
		SocketBufferState state = AllocateBufferState (mainServer, mainEndPoint);
		bool success = false;
		try {
			state.needsCloseWhenDone = true;
			
			serializer.ByteBuffer = state.buffer;
			try {
				NetworkMessageSerializer.Serialize(serializer, disconnectionMessage);
				state.length = serializer.Index;
			}
			catch (Exception e)
			{
				queuedLogs.Add(e);
				return false;
			}
			finally {
				serializer.Clear ();
			}
			
			if (mainSend == null) {
				SimpleSend (state);
			}
			else {
				mainSend.chainedSend = state;
			}
			
			success = true;
			return true;
		}
		catch (Exception e) {
			queuedLogs.Add (e);
			return false;
		}
		finally {
			if (!success) {
				state.Dispose();
			}
		}
	}
	
	private void ServerSend(SocketBufferState state)
	{
		// should indicate programmer error
		if (connectionState != ConnectionState.Connected) {
			throw new InvalidOperationException("Error attempting to send on UdpConnection with state " + connectionState.ToString() + ".");
		}
		
		if (mainSend != null) {
			throw new InvalidOperationException("Error attempting to send on UdpConnection when already sending.");
		}

		mainSend = state;
		bool success = false;
		try {
			AsyncCallback callback = callback_ServerSend_Complete;
			BeginSend (mainSend, callback);
			success = true;
		}
		finally {
			if (!success) {
				mainSend.Dispose();
				mainSend = null;
			}
		}
	}

	private void ServerSend_Complete(IAsyncResult result)
	{
		lock (sync) {
			SocketBufferState state = (SocketBufferState)result.AsyncState;
			bool isMain = false;
			try {
				if (mainSend == state) {
					mainSend = null;
					isMain = true;
				}

				EndSend (result);

				if (!isMain) {
					return;
				}

				if (sentBuffers.Count > 0) {
					SocketBufferState queuedState = sentBuffers.Dequeue();
					bool success = false;
					try {
						ServerSend (queuedState);
						success = true;
					}
					finally {
						if (!success) {
							queuedState.Dispose();
						}
					}
				}
			}
			catch (Exception e) {
				if (isMain) {
				    Destroy (DisconnectionReason.ConnectionLost, e);
				}
				else if (!(e is ObjectDisposedException)) {
					queuedLogs.Add (e);
				}
			}
			finally {
				state.Dispose ();
			}
		}
	}

	private void ServerReceive()
	{
		mainReceive = AllocateBufferState(mainServer, anyEndPoint);
		bool success = false;
		try {
			mainReceive.length = mainReceive.buffer.Length;
			BeginReceive(mainReceive, callback_ServerReceive_Complete);
			success = true;
		}
		finally {
			if (!success) {
				mainReceive.Dispose();
				mainReceive = null;
			}
		}
	}

	private void ServerReceive_Complete(IAsyncResult result) 
	{
		lock (sync) {
			SocketBufferState state = (SocketBufferState)result.AsyncState;
			bool isMain = false;
			if (state == mainReceive) {
				mainReceive = null;
				isMain = true;
			}

			try {
				EndReceive(result);

				if (!isMain) {
					return;
				}

				IPEndPoint ipEndPoint = state.endPoint as IPEndPoint;
				if (ipEndPoint != null) {

					if (connectionState == ConnectionState.Advertising) {
						connectionState = ConnectionState.Connected;
						mainEndPoint = ipEndPoint;
						// ignore first message
					}
					else if (mainEndPoint.Equals (ipEndPoint)) {

						serializer.ByteBuffer = state.buffer;
						NetworkMessage message;
						try {
							NetworkMessageSerializer.Deserialize(serializer, state.length, out message);
						}
						catch (Exception e) {
							Destroy (DisconnectionReason.ReceivedInvalidData, e);
							return;
						}
						finally {
							serializer.Clear ();
						}

						if (receivedMessages.Count >= MaxQueuedReceives) {
							Destroy (DisconnectionReason.ConnectionThrottled,
							       "Too many messages received too quickly. (" + MaxQueuedReceives.ToString() + " messages allowed.)");
							return;
						}
						
						receivedMessages.Add (message);
					}
				}
				
				ServerReceive();
			}
			catch (Exception e) {
				if (isMain) {
					Destroy(DisconnectionReason.ConnectionLost, e);
				}
				else if (!(e is ObjectDisposedException)) {
					queuedLogs.Add (e);
				}
			}
			finally {
				state.Dispose();
			}
		}
	}

	private void SendAdvertisement()
	{
		if (advertisementSend != null || lastAdvertiseTime + AdvertiseTick > lastUpdateTime) {
			return;
		}
		lastAdvertiseTime = lastUpdateTime;

		advertisementSend = AllocateBufferState(advertisementClient, advertisementEndPoint);
		bool success = false;
		try {
			serializer.ByteBuffer = advertisementSend.buffer;
			try {
				advertisementRegistrationMessage.Serialize(serializer);
				advertisementSend.length = serializer.Index;
			}
			catch (IndexOutOfRangeException e) {
				throw new NetworkMessageSerializationException("Send buffer not large enough for AdvertisementRegistrationMsg. " +
				                                               "(" + advertisementRegistrationMessage.SerializationLength.ToString() + " bytes required, " +
				                                               advertisementSend.buffer.Length.ToString() + " bytes allowed in buffer.)",
				                                               e);
			}
			finally {
				serializer.Clear ();
			}

			AsyncCallback callback = callback_SendAdvertisement_Complete;
			BeginSend(advertisementSend, callback);

			success = true;
		}
		finally {
			if (!success) {
				advertisementSend.Dispose();
				advertisementSend = null;
			}
		}
	}

	private void SendAdvertisement_Complete(IAsyncResult result)
	{
		lock (sync) {
			SocketBufferState state = (SocketBufferState)result.AsyncState;
			bool isAdvert = false;
			try {
				if (state == advertisementSend) {
					advertisementSend = null;
					isAdvert = true;
				}

				EndSend(result);
			}
			catch (Exception e) {
				if (isAdvert) {
					Destroy(DisconnectionReason.FailedToAdvertise, e);
				}
				else if (!(e is ObjectDisposedException)) {
					queuedLogs.Add (e);
				}
			}
			finally {
				state.Dispose();
			}
		}
	}

    private SocketBufferState AllocateBufferState(Socket socket, EndPoint endPoint)
	{
		SocketBufferState state;
		int count = bufferStatePool.Count;
		if (count > 0) {
			state = bufferStatePool [count - 1];
			state.Reset (socket, endPoint);
			bufferStatePool.RemoveAt (count - 1);
		} else {
			state = new SocketBufferState (this, socket, endPoint);
		}
		return state;
	}

	private void FreeBufferState(SocketBufferState state)
	{
		if (state.socket == null) {
			return;
		}

		if (bufferStatePool.Count < BufferStatePoolLength) {
			bufferStatePool.Add (state);
		}
	}

	private static void BeginSendUdp(Socket socket, byte[] datagram, int bytes, EndPoint endPoint, AsyncCallback callback, object state)
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

	private void BeginSend(SocketBufferState state, AsyncCallback callback)
	{
		BeginSendUdp (state.socket, state.buffer, state.length, state.endPoint, callback, state);

		pendingOperations++;
	}

	private void EndSend(IAsyncResult result)
	{
		pendingOperations = Math.Max(pendingOperations - 1, 0);

		SocketBufferState state = (SocketBufferState)result.AsyncState;
		if (state.isSocketActive) {
			try {
				int length = state.socket.EndSendTo (result);
				state.length = length;
			}
			catch (ObjectDisposedException) {
				state.length = 0;
				state.isSocketActive = false;
				state.needsCloseWhenDone = false;
				throw;
			}

			if (state.chainedSend != null) {
				SimpleSend(state.chainedSend);
			}
			else if (state.needsCloseWhenDone) {
				state.socket.Close ();
				
				state.isSocketActive = false;
				state.needsCloseWhenDone = false;
			}
		}
	}

	private void BeginReceive(SocketBufferState state, AsyncCallback callback)
	{
		state.socket.BeginReceiveFrom (state.buffer, 0, state.length, SocketFlags.None, ref state.endPoint, callback, state);
		pendingOperations++;
	}

	private void EndReceive(IAsyncResult result)
	{
		pendingOperations = Math.Max(pendingOperations - 1, 0);

		SocketBufferState state = (SocketBufferState)result.AsyncState;
		if (state.isSocketActive) {
			try {
				int length = state.socket.EndReceiveFrom (result, ref state.endPoint);
				state.length = length;
			}
			catch (ObjectDisposedException) {
				state.length = 0;
				state.isSocketActive = false;
				state.needsCloseWhenDone = false;
				throw;
			}

			if (state.needsCloseWhenDone) {
				state.socket.Close ();
				
				state.isSocketActive = false;
				state.needsCloseWhenDone = false;
			}
		}
	}

	private void SimpleSend(SocketBufferState chainedState)
	{
		BeginSend (chainedState, callback_SimpleSend_Complete);
	}
	
	private void SimpleSend_Complete(IAsyncResult result)
	{
		lock (sync) {
			SocketBufferState state = (SocketBufferState)result.AsyncState;
			try {
				EndSend (result);
			} catch (Exception e) {
				queuedLogs.Add (e);
			} finally {
				state.Dispose ();
			}
		}
	}
	
	// adapted from http://stackoverflow.com/a/24814027
	public static IPAddress GetLocalIPv4()
	{
		NetworkInterface[] interfaces = NetworkInterface.GetAllNetworkInterfaces ();
		Array.Sort (interfaces, CompareInterfaces);

		foreach (NetworkInterface item in interfaces)
		{
			foreach (UnicastIPAddressInformation ip in item.GetIPProperties().UnicastAddresses)
			{
				if (ip.Address.AddressFamily == AddressFamily.InterNetwork)
				{
					return (IPAddress)ip.Address;
				}
			}
		}
		return null;
	}

	private static int CompareInterfaces(NetworkInterface a, NetworkInterface b)
	{
		// prioritize anything with status up or unknown
		int compare = -(a.OperationalStatus == OperationalStatus.Up || a.OperationalStatus == OperationalStatus.Unknown).CompareTo (b.OperationalStatus == OperationalStatus.Up || a.OperationalStatus == OperationalStatus.Unknown);
		// then prioritize anything without type loopback nor unknown
		if (compare == 0) compare = -(a.NetworkInterfaceType != NetworkInterfaceType.Loopback && a.NetworkInterfaceType != NetworkInterfaceType.Unknown).CompareTo (b.NetworkInterfaceType != NetworkInterfaceType.Loopback && b.NetworkInterfaceType != NetworkInterfaceType.Unknown);
		// then prioritize anything without type unknown
		if (compare == 0) compare = -(a.NetworkInterfaceType != NetworkInterfaceType.Loopback).CompareTo (b.NetworkInterfaceType != NetworkInterfaceType.Loopback);
		// then prioritize status up over unknown
		if (compare == 0) compare = -(a.OperationalStatus == OperationalStatus.Unknown).CompareTo (b.OperationalStatus == OperationalStatus.Unknown);
		// then be deterministic
		if (compare == 0) compare = a.OperationalStatus.CompareTo (b.OperationalStatus);
		if (compare == 0) compare = a.NetworkInterfaceType.CompareTo (b.NetworkInterfaceType);
		return compare;
	}
}