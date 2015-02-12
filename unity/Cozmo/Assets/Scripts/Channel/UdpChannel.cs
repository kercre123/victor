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
		public readonly byte[] buffer;

		public SocketBufferState(UdpChannel channel, Socket socket)
		{
			this.channel = channel;
			this.socket = socket;
			this.buffer = new byte[MaxBufferSize];
		}

		public void Dispose()
		{
			channel.FreeBufferState (this);
		}
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
		Connected,
		ConnectedAndSending,
	}

	// various constants
	private const int MaxBufferSize = 8192;
	private const int BufferStatePoolLength = 128;
	private const int MaxQueuedSends = 64;
	private const int MaxQueuedReceives = 64;
	private const int MaxQueuedLogs = 64;
	private const float AdvertiseTick = .25f;

	// unique object used for locking
	private readonly object sync = new object();

	// sockets
	private readonly AdvertisementRegistrationMsg advertisementRegistrationMessage = new AdvertisementRegistrationMsg ();
	private readonly IPEndPoint anyEndPoint = new IPEndPoint(IPAddress.Any, 0);
	private Socket advertisingClient;
	private IPEndPoint advertisementEndPoint = null;
	private Socket mainServer;
	private IPEndPoint mainEndPoint = null;

	// state information
	private ConnectionState connectionState = ConnectionState.Disconnected;
	private bool advertisingBusy = false;
	private float lastAdvertise = 0;
	private DisconnectionReason currentDisconnectionReason = DisconnectionReason.ConnectionLost;

	// various queues
	private readonly List<object> queuedLogs = new List<object>(MaxQueuedLogs);
	private readonly Queue<QueuedSend> sentBuffers = new Queue<QueuedSend>(MaxQueuedSends);
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
    
	public override bool HasPendingSends {
		get {
			lock (sync) {
				return (sentBuffers.Count > 0);
			}
		}
	}

	public UdpChannel()
	{
		callback_SendAdvertisement_Complete = SendAdvertisement_Complete;
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
			throw new InvalidOperationException("UdpChannel is already active. Disconnect first.");
		}

		lock (sync) {
			// should only become active through this call
			if (connectionState != ConnectionState.Disconnected) {
				throw new InvalidOperationException("You should only call Connect on the main Unity thread.");
			}

			IPAddress localAddress = GetLocalIPv4();

			try {
				// set up main socket
				IPEndPoint localEndPoint = new IPEndPoint (/*localAddress ?? */ IPAddress.Any, localPort);
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
				advertisingClient = new Socket(advertisementEndPoint.AddressFamily, SocketType.Dgram, ProtocolType.Udp);

				// advertise
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
			if (connectionState != ConnectionState.Connected && connectionState != ConnectionState.ConnectedAndSending) {
				Debug.LogWarning("Ignoring message of type " + message.GetType().FullName + " because the connection is about to disconnect.");
				return;
			}

			bool success = false;
			SocketBufferState state = AllocateBufferState(mainServer);
			try {
				serializer.ByteBuffer = state.buffer;
				int length;
				try {
					NetworkMessageSerializer.Serialize(serializer, message);
					length = serializer.Index;
				}
				catch (Exception e)
				{
					DestroySynchronously(DisconnectionReason.AttemptedToSendInvalidData, e);
					return;
				}
				finally {
					serializer.Clear ();
				}

				if (connectionState == ConnectionState.Connected) {
					try {
						ServerSend (state, length);
						success = true;
					}
					catch (Exception e) {
						DestroySynchronously(DisconnectionReason.ConnectionLost, e);
						return;
					}
				}
				else {
					if (sentBuffers.Count < MaxQueuedSends) {
						sentBuffers.Enqueue (new QueuedSend(state, length));
						success = true;
					}
					else {
						DestroySynchronously(DisconnectionReason.ConnectionThrottled,
						                   "Disconnecting. Too many messages queued to send. (" + MaxQueuedSends.ToString() + " messages allowed.)");
						return;
					}
				}
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
				if (connectionState == ConnectionState.Advertising) {
					try {
						SendAdvertisement();
					}
					catch (Exception e) {
						Destroy(DisconnectionReason.FailedToAdvertise, e);
						return;
					}
				}

				InternalUpdate ();
			}
		}
	}
	
	private void InternalUpdate()
	{
		ProcessLogs();
		
		if (!IsConnected && (connectionState == ConnectionState.Connected || connectionState == ConnectionState.ConnectedAndSending)) {
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
		IsConnected = (connectionState == ConnectionState.Connected || connectionState == ConnectionState.ConnectedAndSending);
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

		connectionState = ConnectionState.Disconnected;
		advertisingBusy = false;
		currentDisconnectionReason = reason;

		sentBuffers.Clear ();
		receivedMessages.Clear ();

		mainEndPoint = null;
		advertisementEndPoint = null;

		if (advertisingClient != null) {
			advertisingClient.Close();
			advertisingClient = null;
		}
		
		if (mainServer != null) {
			mainServer.Close();
			mainServer = null;
		}
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
			state.Dispose();

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
							queuedSend.state.Dispose();
						}
					}
				}
			}
			catch (Exception e) {
				Destroy (DisconnectionReason.ConnectionLost, e);
			}
		}
	}

	private void ServerReceive()
	{
		SocketBufferState state = AllocateBufferState(mainServer);
		bool success = false;
		try {
			EndPoint endPoint = anyEndPoint;
			mainServer.BeginReceiveFrom(state.buffer, 0, state.buffer.Length, SocketFlags.None, ref endPoint, callback_ServerReceive_Complete, state);
			success = true;
		}
		finally {
			if (!success) {
				state.Dispose();
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
					if (ipEndPoint != null) {

						if (connectionState == ConnectionState.Advertising) {
							connectionState = ConnectionState.Connected;
							mainEndPoint = ipEndPoint;
						}
						else if (mainEndPoint.Equals (ipEndPoint)) {
							serializer.ByteBuffer = state.buffer;
							NetworkMessage message;
							try {
								NetworkMessageSerializer.Deserialize(serializer, length, out message);
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
					Destroy(DisconnectionReason.ConnectionLost, e);
				}
			}
			finally {
				state.Dispose();
			}
		}
	}

	private void SendAdvertisement()
	{
		float now = Time.realtimeSinceStartup;
		if (advertisingBusy || lastAdvertise + AdvertiseTick > now) {
			return;
		}
		advertisingBusy = true;
		lastAdvertise = now;

		SocketBufferState state = AllocateBufferState(advertisingClient);
		bool success = false;
		try {
			serializer.ByteBuffer = state.buffer;
			int length;
			try {
				advertisementRegistrationMessage.Serialize(serializer);
				length = serializer.Index;
			}
			catch (IndexOutOfRangeException e) {
				throw new NetworkMessageSerializationException("Send buffer not large enough for AdvertisementRegistrationMsg. " +
				                                               "(" + advertisementRegistrationMessage.SerializationLength.ToString() + " bytes required, " +
				                                               state.buffer.Length.ToString() + " bytes allowed in buffer.)",
				                                               e);
			}
			finally {
				serializer.Clear ();
			}

			AsyncCallback callback = callback_SendAdvertisement_Complete;
			BeginSendUdp(advertisingClient, state.buffer, length, advertisementEndPoint, callback, state);

			success = true;
		}
		finally {
			if (!success) {
				state.Dispose();
				advertisingBusy = false;
			}
		}
	}

	private void SendAdvertisement_Complete(IAsyncResult result)
	{
		lock (sync) {
			SocketBufferState state = (SocketBufferState)result.AsyncState;
			Socket socket = state.socket;
			state.Dispose();

			// this is an old connection
			if (socket != advertisingClient) {
				return;
			}

			advertisingBusy = false;

			try {
				socket.EndSendTo(result);
			}
			catch (Exception e) {
				Destroy(DisconnectionReason.FailedToAdvertise, e);
			}
		}
	}

    private SocketBufferState AllocateBufferState(Socket socket)
	{
		SocketBufferState state;
		int count = bufferStatePool.Count;
		if (count > 0) {
			state = bufferStatePool [count - 1];
			state.socket = socket;
			bufferStatePool.RemoveAt (count - 1);
		} else {
			state = new SocketBufferState (this, socket);
		}
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