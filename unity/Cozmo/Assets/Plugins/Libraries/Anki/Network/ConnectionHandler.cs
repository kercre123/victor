using System.Net.Sockets;

namespace Anki {
  namespace Network {

    // Inmplement this class to handle connections coming from the TCP server
    public abstract class ConnectionHandler {
      private TcpClient _Client;

      public TcpClient Client {
        get  {
          return _Client;
        }

        set {
          _Client = value;
        }
      }

      // The TCPServer starts a new thread when a new connection comes in. This is the starting point for the thread execution.
      public void HandleConnection() {
        try {
          HandleConnectionImpl();
        }
        finally {
          Client.Close();
        }
      }

      protected abstract void HandleConnectionImpl();
    }
  }
}
