using System;
using System.IO;
using System.Net;
using System.Threading;
using System.Windows.Forms;
using Anki.Cozmo.ExternalInterface;
using NetFwTypeLib;

namespace AnimationTool
{
    public static class RobotSettings
    {
        private static readonly object sync = new object();
        public static string AdvertisingIPAddress
        {
            get
            {
                lock (sync)
                {
                    string ip = Properties.Settings.Default.IPAddress;
                    IPAddress address;
                    if (string.IsNullOrEmpty(ip) || !IPAddress.TryParse(ip, out address))
                    {
                        ip = DefaultAdvertisingIPAddress;
                    }
                    return ip;
                }
            }
            set
            {
                if (IsValidIPAddress(value))
                {
                    lock (sync)
                    {
                        Properties.Settings.Default["IPAddress"] = value;
                        Properties.Settings.Default.Save(); // Saves settings in application configuration file
                    }
                }
            }
        }

        public const string DefaultAdvertisingIPAddress = "127.0.0.1";
        public const int AdvertisingPort = 5103;

        public static string VizHostIPAddress
        {
            get
            {
                return AdvertisingIPAddress;
            }
            set
            {
                AdvertisingIPAddress = value;
            }
        }

        public static string RobotIPAddress
        {
            get
            {
                return AdvertisingIPAddress;
            }
            set
            {
                AdvertisingIPAddress = value;
            }
        }

        public const int DeviceId = 1;
        public const int RobotId = 1;
        public const bool RobotIsSimulated = true;

        public const int MinimumPort = 5200;
        public const int MaximumPort = 5299;

        private static int memoryPort = MinimumPort;
        private static int CurrentPort
        {
            get
            {
                int port = Properties.Settings.Default.LocalPort;
                if (port >= MinimumPort && port <= MaximumPort)
                {
                    return port;
                }
                port = memoryPort;
                if (port >= MinimumPort && port <= MaximumPort)
                {
                    return port;
                }
                return MinimumPort;
            }
            set
            {
                if (value < MinimumPort || value > MaximumPort)
                {
                    return;
                }

                memoryPort = value;
                Properties.Settings.Default["LocalPort"] = value;
                Properties.Settings.Default.Save();
            }
        }

        public static bool IsValidIPAddress(string ip)
        {
            IPAddress address;
            return (!string.IsNullOrEmpty(ip) && IPAddress.TryParse(ip, out address));
        }

        public static int GetNextFreePort()
        {
            OpenFirewallPorts(MinimumPort, MaximumPort);
            lock (sync)
            {
                int port = CurrentPort + 1;
                if (port > MaximumPort)
                {
                    port = MinimumPort;
                }
                CurrentPort = port;
                return port;
            }
        }

        /// <summary>
        /// See http://stackoverflow.com/a/13386038/4093018
        /// </summary>
        private static void OpenFirewallPorts(int portMinimum, int portMaximum)
        {
            Type netFwMgrType = Type.GetTypeFromProgID("HNetCfg.FwMgr", false);

            if (netFwMgrType != null)
            {
                INetFwMgr mgr = (INetFwMgr)Activator.CreateInstance(netFwMgrType);

                if (mgr != null)
                {
                    INetFwOpenPorts ports = mgr.LocalPolicy.CurrentProfile.GloballyOpenPorts;

                    if (ports != null)
                    {
                        Type authPortType = Type.GetTypeFromProgID("HNetCfg.FWOpenPort", false);

                        if (authPortType != null)
                        {
                            for (int i = portMinimum; i <= portMaximum; ++i)
                            {

                                INetFwOpenPort port = (INetFwOpenPort)Activator.CreateInstance(authPortType);

                                if (port != null)
                                {
                                    port.Protocol = NET_FW_IP_PROTOCOL_.NET_FW_IP_PROTOCOL_UDP;
                                    port.Port = i;
                                    port.Name = Path.GetFileName(Application.ExecutablePath);
                                    port.Scope = NET_FW_SCOPE_.NET_FW_SCOPE_ALL;
                                    ports.Add(port);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
